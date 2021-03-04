

#include "pch.h"
#include "CommonConnectionPool.h"
#include "public.h"

// 线程安全的懒汉单例函数接口
ConnectionPool* ConnectionPool::getConnectionPool()
{
	static ConnectionPool pool; //对于静态变量的初始化,编译器自动 lock和unlock
	return &pool;
}

// 从配置文件中加载配置项
bool ConnectionPool::loadConfigFile()
{
	FILE *pf = fopen("mysql.ini", "r");//r是只读
	if (pf == nullptr)
	{
		LOG("mysql.ini file is not exist!");
		return false;
	}

	while (!feof(pf))//检测流上的文件结束符文件结束返回非0，只要文件没有接收到数据
	{
		char line[1024] = { 0 };
		fgets(line, 1024, pf);//一行一行读取
		string str = line;
		int idx = str.find('=', 0);//找到=号
		if (idx == -1) // 无效的配置项
		{
			continue;
		}

		// password=hlt12345\n实际上还有换行符号
		int endidx = str.find('\n', idx);//找到\n的位置
		string key = str.substr(0, idx);
		string value = str.substr(idx + 1, endidx - idx - 1);

		if (key == "ip")
		{
			_ip = value;
		}
		else if (key == "port")
		{
			_port = atoi(value.c_str());//字符串转换从整数
		}
		else if (key == "username")
		{
			_username = value;
		}
		else if (key == "password")
		{
			_password = value;
		}
		else if (key == "dbname")
		{
			_dbname = value;
		}
		else if (key == "initSize")
		{
			_initSize = atoi(value.c_str());
		}
		else if (key == "maxSize")
		{
			_maxSize = atoi(value.c_str());
		}
		else if (key == "maxIdleTime")
		{
			_maxIdleTime = atoi(value.c_str());
		}
		else if (key == "connectionTimeOut")
		{
			_connectionTimeout = atoi(value.c_str());
		}
	}
	return true;//加载完了返回true
}

// 连接池的构造
ConnectionPool::ConnectionPool()
{
	// 加载配置项出错直接返回
	if (!loadConfigFile())
	{
		return;
	}

	// 创建初始数量的连接
	for (int i = 0; i < _initSize; ++i)
	{
		Connection *p = new Connection();
		p->connect(_ip, _port, _username, _password, _dbname);//创建新的连接
		p->refreshAliveTime(); // 刷新一下开始空闲的起始时间,进入队列即为使用连接池连接的起始时间
		_connectionQue.push(p);//添加到队列里
		_connectionCnt++;//连接计数加1
	}

	// 启动一个新的线程，作为连接的生产者 linux thread => pthread_create
	thread produce(std::bind(&ConnectionPool::produceConnectionTask, this));//绑定函数地址和当前对象
	produce.detach();//设置成分离线程

	// 启动一个新的定时线程，扫描超过maxIdleTime时间的空闲连接，进行多余空闲的连接回收
	thread scanner(std::bind(&ConnectionPool::scannerConnectionTask, this));
	scanner.detach();
}

// 运行在独立的线程中，专门负责生产新连接
void ConnectionPool::produceConnectionTask()
{
	for (;;)
	{
		unique_lock<mutex> lock(_queueMutex);//互斥锁使用
		while (!_connectionQue.empty())//如果为空才生产
		{
			cv.wait(lock); // 队列不空，此处生产线程进入等待状态
		}

		// 连接数量没有到达上限，继续创建新的连接
		if (_connectionCnt < _maxSize)//超过了maxSize不创建
		{
			Connection *p = new Connection();
			p->connect(_ip, _port, _username, _password, _dbname);//连接数据库
			p->refreshAliveTime(); // 刷新一下开始空闲的起始时间
			_connectionQue.push(p);//放入到队列中
			_connectionCnt++;
		}

		// 通知消费者线程，可以消费连接了
		cv.notify_all();
	}
}

// 给外部提供接口，从连接池中获取一个可用的空闲连接
shared_ptr<Connection> ConnectionPool::getConnection()
{
	unique_lock<mutex> lock(_queueMutex);//操作队列前加锁
	while (_connectionQue.empty())//队列是空的
	{
		// sleep是直接睡多少时间在这里用不合适
		if (cv_status::timeout == cv.wait_for(lock, chrono::milliseconds(_connectionTimeout)))//等待一个超时时间,
		{	//timeout是保证在等待过程中出现被抢占的情况不属于获取空闲连接超时
			if (_connectionQue.empty())//队列依然是空的
			{
				LOG("获取空闲连接超时了...获取连接失败!");
					return nullptr;
			}
		}
	}
	//不为空直接进入下面的消费过程
	/*
	shared_ptr智能指针析构时，会把connection资源直接delete掉，相当于
	调用connection的析构函数，connection就被close掉了。
	这里需要自定义shared_ptr的释放资源的方式，把connection直接归还到queue当中
	*/
	shared_ptr<Connection> sp(_connectionQue.front(), 
		[&](Connection *pcon) {//pcon是指向连接的指针
		// 这里是在服务器应用线程中调用的，所以一定要考虑队列的线程安全操作
		unique_lock<mutex> lock(_queueMutex);//保证线程安全
		pcon->refreshAliveTime(); // 刷新一下开始空闲的起始时间
		_connectionQue.push(pcon);//指针添加到队列里归还连接池连接
	});

	_connectionQue.pop();//出队让消费者消费
	cv.notify_all();  // 消费完连接以后，通知生产者线程检查一下，如果队列为空了，赶紧生产连接。
	
	return sp;
}
//最大空闲时间回收连接扫描线程
// 扫描超过maxIdleTime时间的空闲连接，进行对于的连接回收
void ConnectionPool::scannerConnectionTask()
{
	for (;;)
	{
		// 通过sleep模拟定时效果
		this_thread::sleep_for(chrono::seconds(_maxIdleTime));

		// 扫描整个队列，释放多余的连接
		unique_lock<mutex> lock(_queueMutex);
		while (_connectionCnt > _initSize)
		{
			Connection *p = _connectionQue.front();//只要队头的时间没有超时那么队列中的一定没有超时
			if (p->getAliveTime() >= (_maxIdleTime * 1000))
			{
				_connectionQue.pop();//出队
				_connectionCnt--;//连接总数减1
				delete p; // 调用~Connection()释放连接
			}
			else
			{
				break; // 队头的连接没有超过_maxIdleTime，其它连接肯定没有
			}
		}
	}
}