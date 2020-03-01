# KyImServer
简单实现了一个IM服务器，具备基本的注册、登录、好友、群组、聊天、文件功能。<br>
参考[**flamingo**](https://github.com/balloonwj/flamingo)来实现的，并且是用**flamingo**的客户端来进行测试的。

和**flamingo**服务端的对比区别如下：

1. 数据库：**KyImServer**用C++封装了`hiredis`接口，**flamingo**使用的`Mysql`。
2. 网络通信框架：**KyImServer**用C++封装了`libevent`，**flamingo**应该是使用了`muduo`的部分代码。
3. 日志系统逻辑**KyImServer**和**flamingo**差不多，都是多线程生产者消费者模式。
4. 客户端服务端的通信协议、包的封装和解析都是直接用的**flamingo**的。
5. 业务逻辑有较大区别，例如在**KyImServer**中：用户和群组分开用两个不同的结构存储、好友列表用类对象来存储等。
6. 配置文件用的是**flamingo**的，自己写了读取配置文件的类`CConfigFile`。

###1、开发工具
使用CLion开发，远程到服务器上运行调试。
###2、使用到的第三方库
- jsoncpp和zlib都以源码的方式放到项目中了，其中zlib是从flamingo拿过来的
- libevent-2.1.8-stable
- hiredis-0.14.0
- 用redis-5.0.5当作数据库
###3、Libevent的封装
刚开始封装的时候遇到了个问题，因为Libevent是C语言写的，而C++类普通成员函数无法作为C语言的函数指针参数，查了很久发现只能通过回调参数把类对象的指针传过去，再通过对象进行函数回调。

- `CEventLoop`：对Libevent的event_base的简单包装。
- `CTcpServer`：添加服务器socket监听事件到主循环上，创建工作线程，当有新的客户端连接到来时通过管道告诉工作线程处理新的客户端的事件。
- `CWorkThread`：每个工作线程也都运行一个事件循环，接收主线程发过来的新客户端的fd，为这个fd添加各种事件，并挂到当前工作线程的事件循环上进行监听，每有一个新连接，就创建一个新连接的对象，连接对象设置好所需要事件的回调函数即可。当发生事件时由连接的函数成员进行回调。
- `CTcpConnection`：回调给用户的参数，包含Libevent的输入输出buffer，暴露接口给用户调用，读取和发送数据。

参考：[博客](https://www.tuicool.com/articles/QBj2ma)和[muduo](https://github.com/chenshuo/muduo)
###4、hiredis的封装
封装redis时遇到的一个问题，一开始是用`redisCommand`来发送命令，当键或者值有空格的时候，构造的命令就会出现参数个数错误的问题。找到两种解决办法：

1. 可以用`%b`这种二进制安全的占位符解决。
2. 使用`redisCommandArgv`来构造命令参数。

因为在实际情况下，要使用第一种构造包含不定个数的参数时，基本上没法实现，所以后来选择了第二种方法。

对redis的封装只是提供了一些接口，接口名是对应的redis命令，接口内部实现好参数的构造等。简化了调用时的操作。

###5、日志系统
日志系统使用生产者消费者模式实现。提供日志接口LOGE、LOGI、LOGD等，调用后将日志内容构造成指定的格式，然后加入到待写日志队列中，用条件变量通知消费者线程去取出待写日志，然后写入到文件中。

