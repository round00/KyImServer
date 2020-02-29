//
// Created by gjk on 2020/2/22.
//

#include "FileServer.h"
#include "Logger.h"
#include "FileSession.h"

bool CFileServer::init(CEventLoop *loop, int listenport, int thread) {
    setConnectCallBack(std::bind(&CFileServer::onConnected, this, std::placeholders::_1));
    setCloseCallBack(std::bind(&CFileServer::onClosed, this, std::placeholders::_1));
    return CTcpServer::init(loop, listenport, thread);
}

void CFileServer::onConnected(CTcpConnection *conn) {
//    fprintf(stderr, "new Connection from %d\n", conn->m_clientFd);
    std::shared_ptr<CFileSession> session(new CFileSession(conn));
    conn->setMessageCallBack(std::bind(&CFileSession::onMessage, session.get(), std::placeholders::_1));

    CMutexLock lock(m_mutex);
    m_sessions[conn->m_clientFd] = session;
    LOGD("The number of online connections is %d", m_sessions.size());
}

void CFileServer::onClosed(CTcpConnection *conn) {
    CMutexLock lock(m_mutex);
    auto it = m_sessions.find(conn->m_clientFd);
    if(it != m_sessions.end()) {
        m_sessions.erase(it);
    }

    LOGD("The number of online connections is %d", m_sessions.size());
}