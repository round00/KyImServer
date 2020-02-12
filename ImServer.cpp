//
// Created by gjk on 2020/2/10.
//

#include "ImServer.h"
#include "ClientSession.h"

bool CImServer::init(CEventLoop *loop, int listenport, int thread) {
    setConnectCallBack(std::bind(&CImServer::onConnected, this, std::placeholders::_1));
    setCloseCallBack(std::bind(&CImServer::onClosed, this, std::placeholders::_1));
    return CTcpServer::init(loop, listenport, thread);
}

void CImServer::onConnected(CTcpConnection *conn) {
    fprintf(stderr, "new Connection from %d\n", conn->m_clientFd);
    std::shared_ptr<CClientSession> session(new CClientSession(conn));
    conn->setMessageCallBack(std::bind(&CClientSession::onMessage, session.get(), std::placeholders::_1));
    m_sessions[conn->m_clientFd] = session;
}

void CImServer::onClosed(CTcpConnection *conn) {
    auto it = m_sessions.find(conn->m_clientFd);
    if(it != m_sessions.end()){
        m_sessions.erase(it);
    }
}