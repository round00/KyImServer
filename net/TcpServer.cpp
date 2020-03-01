//
// Created by gjk on 2019/12/30.
//

#include <string.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "TcpServer.h"
#include "EventLoop.h"


CTcpConnection::CTcpConnection(int fd,CWorkThread* worker):
m_clientFd(fd), m_worker(worker),
m_inputBuf(nullptr),m_outputBuf(nullptr),
m_connectCB(nullptr),m_messageCB(nullptr),m_closeCB(nullptr)
{}

//==================================================
CWorkThread::CWorkThread(CTcpServer *tcpServer) :
m_evBase(nullptr),m_tcpServer(tcpServer)
{
}

CWorkThread::~CWorkThread()
{
    //关闭线程
    close();
    //关闭连接，释放连接对象
    for(auto it:m_conns){
        ::close(it.first);
        delete it.second;
    }
    //关闭管道描述符
    ::close(m_fdWrite);
    ::close(m_fdRead);
}

bool CWorkThread::start() {
    //创建当前线程的事件循环，用于接收任务
    m_evBase = event_base_new();
    if(!m_evBase){
        return false;
    }
    //创建管道，用于向当前线程的事件循环发送任务
    int pipefd[2];
    if(::pipe(pipefd) != 0){
        return false;
    }
    m_fdRead = pipefd[0];
    m_fdWrite = pipefd[1];
    //创建监听管道的事件
    struct event* ev = event_new(m_evBase, m_fdRead, EV_READ|EV_PERSIST, &CWorkThread::processFunc, this);
    if(!ev){
        return false;
    }
    if(event_add(ev, nullptr)!=0){
        return false;
    }

    //启动线程
    ::pthread_create(&m_threadId, nullptr, &CWorkThread::threadFunc, this);
    return true;
}

void CWorkThread::close() {
    process(-1);
}

void CWorkThread::process(int fd) {
    ::write(m_fdWrite, &fd, sizeof(fd));
}

void* CWorkThread::threadFunc(void *arg) {
    CWorkThread* worker = (CWorkThread*)arg;
    event_base_dispatch(worker->m_evBase);

    return nullptr;
}

void CWorkThread::processFunc(evutil_socket_t fd, short what, void *arg) {
    CWorkThread* worker = (CWorkThread*)arg;
    int clientFd;
    ::read(fd, &clientFd, sizeof(clientFd));
    //传-1过来，意思是要退出线程
    if(clientFd==-1){
        event_base_loopbreak(worker->m_evBase);
        return;
    }

//    fprintf(stderr, "There is a new connection.\n");
    //建立和客户端的连接
    struct bufferevent* bev = bufferevent_socket_new(
            worker->m_evBase, clientFd, BEV_OPT_CLOSE_ON_FREE);
    //连接建立失败
    if(!bev){
        return;
    }
    //建立连接成功，创建连接的对象，这个对象用于后续的回调操作
    CTcpConnection*conn = worker->addConnection(clientFd);
    bufferevent_setcb(bev, &CWorkThread::readEvent, &CWorkThread::writeEvent, &CWorkThread::closeEvent, conn);
    bufferevent_enable(bev, EV_READ);
    bufferevent_enable(bev, EV_WRITE);

    //连接建立的回调
    if(conn->m_connectCB){
        conn->m_connectCB(conn);
    }
}

void CWorkThread::readEvent(struct bufferevent *bev, void *ctx) {
    CTcpConnection* conn = (CTcpConnection*)ctx;
    conn->m_inputBuf = bufferevent_get_input(bev);
    conn->m_outputBuf = bufferevent_get_output(bev);
    if(conn->m_messageCB){
        conn->m_messageCB(conn);
    }
}

void CWorkThread::writeEvent(struct bufferevent *bev, void *ctx) {
    CTcpConnection* conn = (CTcpConnection*)ctx;
    conn->m_inputBuf = bufferevent_get_input(bev);
    conn->m_outputBuf = bufferevent_get_output(bev);
//    conn->m_worker->m_tcpServer->onMessage(conn);
}

void CWorkThread::closeEvent(struct bufferevent *bev, short what, void *ctx) {
    CTcpConnection* conn = (CTcpConnection*)ctx;

    if(conn->m_closeCB){
        conn->m_closeCB(conn);
    }

    conn->m_worker->delConnection(conn);
}

CTcpConnection* CWorkThread::addConnection(int clientFd) {
    CTcpConnection* conn = new CTcpConnection(clientFd, this);
    conn->setConnectCallBack(m_tcpServer->getConnectCallBack());
    conn->setCloseCallBack(m_tcpServer->getCloseCallBack());

    m_conns[clientFd] = conn;
    return conn;
}

void CWorkThread::delConnection(CTcpConnection *conn) {
    if(!conn){
        return;
    }
    auto it = m_conns.find(conn->m_clientFd);
    if(it == m_conns.end()){
        return;
    }

    m_conns.erase(it);
    delete conn;
}



//==================================================
CTcpServer::CTcpServer():
m_listener(nullptr),m_threadCount(1)
{
}

CTcpServer::~CTcpServer() {
    if(m_listener){
        evconnlistener_free(m_listener);
    }

    for(auto worker : m_workers){
        delete worker;
    }
    m_workers.clear();
}

bool CTcpServer::init(CEventLoop *loop, int listenport, int thread) {
    //绑定监听端口到主循环上
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(listenport);
    m_listener = evconnlistener_new_bind(loop->getBase(),
            &CTcpServer::newConnEvent, this, LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE,
            -1, (struct sockaddr*)&sin, sizeof sin);
    //启动工作线程
    m_threadCount = thread;
    if(m_threadCount<1){
        m_threadCount = 1;
    }
    for(int i = 0;i<m_threadCount; ++i){
        CWorkThread* worker = new CWorkThread(this);
        worker->start();
        m_workers.push_back(worker);
    }

    return true;
}

void CTcpServer::newConnEvent(struct evconnlistener *lev,
                        evutil_socket_t fd,
                        struct sockaddr *addr,
                        int socklen,
                        void *arg)
{
    CTcpServer* server = (CTcpServer*)arg;
    int i = rand()%server->m_threadCount;
    server->m_workers[i]->process(fd);
}

