#include "pch.h"
#include <iostream>
using namespace std;
#include "Connection.h"
#include "CommonConnectionPool.h"

int main()
{
	Connection conn;
	conn.connect("127.0.0.1", 3306, "root", "hlt12345", "chat");

	/*Connection conn;
	char sql[1024] = { 0 };
	sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
		"zhang san", 20, "male");
	conn.connect("127.0.0.1", 3306, "root", "123456", "chat");
	conn.update(sql);*/

	clock_t begin = clock();//初始时间
	
	thread t1([]() {
		//ConnectionPool *cp = ConnectionPool::getConnectionPool();
		for (int i = 0; i < 2500; ++i)
		{
			/* 不使用连接池进行Mysql操作
				char sql[1024] = { 0 };
				sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
				"zhang san", 20, "male");
				conn.connect("127.0.0.1",3306, "root", "hlt12345", "chat");
				conn.update(sql);
			*/
			/*char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
				"zhang san", 20, "male");
			shared_ptr<Connection> sp = cp->getConnection();
			sp->update(sql);*/
			Connection conn;
			char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
				"zhang san", 20, "male");
			conn.connect("127.0.0.1", 3306, "root", "hlt12345", "chat");
			conn.update(sql);
		}
	});
	thread t2([]() {
		//ConnectionPool *cp = ConnectionPool::getConnectionPool();
		for (int i = 0; i < 2500; ++i)
		{
			/*char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
				"zhang san", 20, "male");
			shared_ptr<Connection> sp = cp->getConnection();
			sp->update(sql);*/
			Connection conn;
			char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
				"zhang san", 20, "male");
			conn.connect("127.0.0.1", 3306, "root", "hlt12345", "chat");
			conn.update(sql);
		}
	});
	thread t3([]() {
		//ConnectionPool *cp = ConnectionPool::getConnectionPool();
		for (int i = 0; i < 2500; ++i)
		{
			/*char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
				"zhang san", 20, "male");
			shared_ptr<Connection> sp = cp->getConnection();
			sp->update(sql);*/
			Connection conn;
			char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
				"zhang san", 20, "male");
			conn.connect("127.0.0.1", 3306, "root", "hlt12345", "chat");
			conn.update(sql);
		}
	});
	thread t4([]() {
		//ConnectionPool *cp = ConnectionPool::getConnectionPool();
		for (int i = 0; i < 2500; ++i)
		{
			/*char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
				"zhang san", 20, "male");
			shared_ptr<Connection> sp = cp->getConnection();
			sp->update(sql);*/
			Connection conn;
			char sql[1024] = { 0 };
			sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
				"zhang san", 20, "male");
			conn.connect("127.0.0.1", 3306, "root", "hlt12345", "chat");
			conn.update(sql);
		}
	});

	t1.join();
	t2.join();
	t3.join();
	t4.join();

	clock_t end = clock();//结束时间
	cout << (end - begin) << "ms" << endl;//打印连接所花的时间


#if 0
	for (int i = 0; i < 10000; ++i)
	{
		Connection conn;
		char sql[1024] = { 0 };
		sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
			"zhang san", 20, "male");
		conn.connect("127.0.0.1", 3306, "root", "123456", "chat");
		conn.update(sql);

		/*shared_ptr<Connection> sp = cp->getConnection();
		char sql[1024] = { 0 };
		sprintf(sql, "insert into user(name,age,sex) values('%s',%d,'%s')",
			"zhang san", 20, "male");
		sp->update(sql);*/
	}
#endif

	return 0;
}
