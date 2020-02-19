# KyImServer
简单实现了一个IM服务器，具备基本的注册、登录、好友、群组、聊天、文件功能。<br>
参考[**flamingo**](https://github.com/balloonwj/flamingo)来实现的，并且是用**flamingo**的客户端来进行测试的。

和**flamingo**服务端的对比区别如下：

1. 数据库：**KyImServer**用C++封装了`hiredis`接口，**flamingo**使用的`Mysql`。
2. 网络通信框架：**KyImServer**用C++封装了`libevent`，**flamingo**应该是使用了`muduo`的部分代码
3. 日志系统逻辑**KyImServer**和**flamingo**差不多，都是多线程生产者消费者模式。
4. 客户端服务端的通信协议、包的封装和解析都是直接用的**flamingo**的。
5. 业务逻辑有较大区别，例如在**KyImServer**中：用户和群组分开用两个不同的结构存储、好友列表用类对象来存储等。