//
// Created by gjk on 2019/12/30.
// libevent封装成一个TcpServer类，实现参考了
// https://www.tuicool.com/articles/QBj2ma
//

#ifndef KYIMSERVER_TCPSERVER_H
#define KYIMSERVER_TCPSERVER_H
#include <event.h>
#include <event2/listener.h>
#include <string>
#include <memory>
#include <list>
#include <vector>
#include <map>
#include <functional>

class CEventLoop;
class CTcpServer;
class CWorkThread;
class CTcpConnection;
//==================================================
//这个类是回调给用户接口的参数，供用户获取连接的信息和提取数据

typedef std::function<void (CTcpConnection* conn)> ConnectCallBack;
typedef std::function<void (CTcpConnection* conn)> MessageCallBack;
typedef std::function<void (CTcpConnection* conn)> CloseCallBack;

class CTcpConnection{
public:
    CTcpConnection(int fd,CWorkThread* worker);

    void    setConnectCallBack(const ConnectCallBack& cb) {m_connectCB = cb;}
    void    setMessageCallBack(const ConnectCallBack& cb) {m_messageCB = cb;}
    void    setCloseCallBack(const ConnectCallBack& cb) {m_closeCB = cb;}
    //获取可读长度
    size_t readableLength(){
        return evbuffer_get_length(m_inputBuf);
    }

    //将输入数据取出放到buffer中，返回取出的数据量
    int retriveBuffer(char *buffer, int len){
        return evbuffer_remove(m_inputBuf, buffer, len);
    }
    int retriveBuffer(std::string& buffer, int len){
        char *buf = new char[len];
        int nLen = evbuffer_remove(m_inputBuf, buf, len);
        buffer = std::string(buf, nLen);
        delete[] buf;
        return nLen;
    }

    //将输入数据复制一份到buffer中，返回复制的数据量
    size_t peekBuffer(char *buffer, int len){
        return evbuffer_copyout(m_inputBuf, buffer, len);
    }

    //获取可写长度
    size_t writeableLength(){
        return evbuffer_get_length(m_outputBuf);
    }

    //将数据放入写缓冲区
    int addWriteBuffer(const char *buffer, int len){
        return evbuffer_add(m_outputBuf, buffer, len);
    }

    int addWriteBuffer(const std::string& buffer){
        return evbuffer_add(m_outputBuf, buffer.c_str(), buffer.length());
    }

    //将输入缓冲区的数移动到输出缓冲区
    void moveInToOut(){
        evbuffer_add_buffer(m_outputBuf, m_inputBuf);
    }

    int                 m_clientFd;
    CWorkThread*        m_worker;
    struct evbuffer*    m_inputBuf;
    struct evbuffer*    m_outputBuf;

    ConnectCallBack     m_connectCB;
    MessageCallBack     m_messageCB;
    CloseCallBack       m_closeCB;
};

//==================================================
class CWorkThread{
public:
    CWorkThread(CTcpServer* tcpServer);
    ~CWorkThread();

    bool                start();
    void                close();
    //通知当前线程处理任务，传fd=-1表示关闭事件循环
    void                process(int fd);
    //线程函数，用来跑事件循环
    static void*        threadFunc(void* arg);
    //处理函数，处理任务的具体位置
    static void         processFunc(evutil_socket_t fd, short what, void *arg);

    //各个事件的回调
    static void         readEvent(struct bufferevent *bev, void *ctx);
    static void         writeEvent(struct bufferevent *bev, void *ctx);
    static void         closeEvent(struct bufferevent *bev, short what, void *ctx);

    CTcpConnection*     addConnection(int clientFd);
    void                delConnection(CTcpConnection* conn);

private:
    event_base*         m_evBase;
    //主线程通过管道向工作线程发送消息，来通知工作线程有任务
    //FIXME:其实这里没必要用管道，直接用生产者消费者模式就可以了
    int                 m_fdRead;
    int                 m_fdWrite;
    CTcpServer*         m_tcpServer;
    pthread_t           m_threadId;
    //保存当前所有连接的列表
    std::map<int, CTcpConnection*>  m_conns;

};


//==================================================
//创建一个派生类继承这个类，然后：
// 1.调用init
// 2.重载需要的事件
class CTcpServer{
public:
    CTcpServer();
    ~CTcpServer();
    //将CTcpServer挂到loop事件循环上，listenport是这个tcpserver监听的端口
    virtual bool        init(CEventLoop* loop, int listenport, int thread);
    static void         newConnEvent(struct evconnlistener *lev,
                                     evutil_socket_t fd, struct sockaddr *addr, int socklen, void *arg);

    void                setConnectCallBack(const ConnectCallBack& cb){m_connectCB = cb;}
    ConnectCallBack&    getConnectCallBack(){return m_connectCB;}

    void                setCloseCallBack(const CloseCallBack& cb){m_closeCB = cb;}
    CloseCallBack&      getCloseCallBack(){return m_closeCB;}

    //派生类重写下面的函数即可
//    virtual void        onConnected(CTcpConnection* conn){}
//    virtual void        onMessage(CTcpConnection* conn){}
//    virtual void        onConnClosed(CTcpConnection* conn){}

private:

    struct evconnlistener*      m_listener;
    //工作线程
    std::vector<CWorkThread*>   m_workers;
    //多少个工作线程
    int                         m_threadCount;
    ConnectCallBack             m_connectCB;
    CloseCallBack               m_closeCB;
};


#endif //KYIMSERVER_TCPSERVER_H
