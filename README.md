# MysqlConnectPool
提高MySQL数据库（基于C/S设计）的访问瓶颈，除了在服务器端增加缓存服务器缓存常用的数据 之外（例如redis），还可以增加连接池，来提高MySQL Server的访问效率，在高并发情况下，大量的 TCP三次握手、MySQL Server连接认证、MySQL Server关闭连接回收资源和TCP四次挥手所耗费的 性能时间也是很明显的，增加连接池就是为了减少这一部分的性能损耗。 

# 连接池功能

连接池一般包含了数据库连接所用的ip地址、port端口号、用户名和密码以及其它的性能参数，例如初 始连接量，最大连接量，最大空闲时间、连接超时时间等，该项目是基于C++语言实现的连接池，主要 也是实现以上几个所有连接池都支持的通用基础功能。

**初始连接量（initSize）**：表示连接池事先会和MySQL Server创建initSize个数的connection连接，当 应用发起MySQL访问时，不用再创建和MySQL Server新的连接，直接从连接池中获取一个可用的连接 就可以，使用完成后，并不去释放connection，而是把当前connection再归还到连接池当中。 

**最大连接量（maxSize）**：当并发访问MySQL Server的请求增多时，初始连接量已经不够使用了，此 时会根据新的请求数量去创建更多的连接给应用去使用，但是新创建的连接数量上限是maxSize，不能 无限制的创建连接，因为每一个连接都会占用一个socket资源，一般连接池和服务器程序是部署在一台 主机上的，如果连接池占用过多的socket资源，那么服务器就不能接收太多的客户端请求了。当这些连 接使用完成后，再次归还到连接池当中来维护。

**最大空闲时间（maxIdleTime）**：当访问MySQL的并发请求多了以后，连接池里面的连接数量会动态 
增加，上限是maxSize个，当这些连接用完再次归还到连接池当中。如果在指定的maxIdleTime里面， 这些新增加的连接都没有被再次使用过，那么新增加的这些连接资源就要被回收掉，只需要保持初始连 接量initSize个连接就可以了。

**连接超时时间（connectionTimeout）**：当MySQL的并发请求量过大，连接池中的连接数量已经到达 
maxSize了，而此时没有空闲的连接可供使用，那么此时应用从连接池获取连接无法成功，它通过阻塞 的方式获取连接的时间如果超过connectionTimeout时间，那么获取连接失败，无法访问数据库。


主要实现了这4个功能。

# 功能和设计

- ConnectionPool.cpp和ConnectionPool.h：连接池代码实现 。
- Connection.cpp和Connection.h：数据库操作代码、增删改查代码实现 。
- main.cpp是测试代码实现了通过多线程同时对数据库进行插入操作然后返回插入所用的时间。
- mysql.ini是配置文件从中获取数据库的ip,用户名等信息,可以让你快速使用。
- 在Mysql中新建了一个数据库chat，在该数据库下新建了一个user表来测试线程池。
- public.c中封装了一个LOG宏定义用来打印出错信息。

注:由于
```
+-------+-----------------------+------+-----+---------+----------------+
| Field | Type                  | Null | Key | Default | Extra          |
+-------+-----------------------+------+-----+---------+----------------+
| id    | int(11)               | NO   | PRI | NULL    | auto_increment |
| name  | varchar(50)           | YES  |     | NULL    |                |
| age   | int(11)               | YES  |     | NULL    |                |
| sex   | enum('male','female') | YES  |     | NULL    |                |
+-------+-----------------------+------+-----+---------+----------------+
```
连接池主要包含了以下功能点： 
1. 连接池只需要一个实例，所以ConnectionPool以单例模式进行设计 
2. 从ConnectionPool中可以获取和MySQL的连接Connection 
3. 空闲连接Connection全部维护在一个线程安全的Connection队列中，使用线程互斥锁保证队列的线 程安全 
4. 如果Connection队列为空，还需要再获取连接，此时需要动态创建连接，上限数量是maxSize 
5. 队列中空闲连接时间超过maxIdleTime的就要被释放掉，只保留初始的initSize个连接就可以了，这个 功能点肯定需要放在独立的线程中去做 
6. 如果Connection队列为空，而此时连接的数量已达上限maxSize，那么等待connectionTimeout时间 如果还获取不到空闲的连接，那么获取连接失败，此处从Connection队列获取空闲连接，可以使用带 超时时间的mutex互斥锁来实现连接超时时间 
7. 用户获取的连接用shared_ptr智能指针来管理，用lambda表达式定制连接释放的功能（不真正释放 连接，而是把连接归还到连接池中） 
8. 连接的生产和连接的消费采用生产者-消费者线程模型来设计，使用了线程间的同步通信机制条件变量 和互斥锁 


# Windows和Linux下开发(安装Mysql5.7)
如果在windows端使用visual studio需要进行相应的头文件和库文件的配置:
1. 右键项目 - C/C++ - 常规 - 附加包含目录，填写mysql.h头文件的路径 
2. 右键项目 - 链接器 - 常规 - 附加库目录，填写libmysql.lib的路径
3. 右键项目 - 链接器 - 输入 - 附加依赖项，填写libmysql.lib库的名字 
4. 把libmysql.dll动态链接库（Linux下后缀名是.so库）放在工程目录下

Linux端的需要
把libmysql.so添加到LD_LIBRARY_PATH动态库路径中。或者在gcc编译的时候
`gcc main.c -I ./ -L ./ -l test -o app`
注:-I后面的./是头文件，-L 后面的./是动态库路径,-l后面的test是动态库名称。