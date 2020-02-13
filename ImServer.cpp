//
// Created by gjk on 2020/2/10.
//

#include "ImServer.h"
#include "ClientSession.h"
#include "Msg.h"

bool CImServer::init(CEventLoop *loop, int listenport, int thread) {
    setConnectCallBack(std::bind(&CImServer::onConnected, this, std::placeholders::_1));
    setCloseCallBack(std::bind(&CImServer::onClosed, this, std::placeholders::_1));
    return CTcpServer::init(loop, listenport, thread);
}

void CImServer::onConnected(CTcpConnection *conn) {
//    fprintf(stderr, "new Connection from %d\n", conn->m_clientFd);
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

std::shared_ptr<CClientSession>
        CImServer::getClientSession(uint32_t uid, uint8_t client) {
    for(const auto& it:m_sessions){
        auto user = it.second->getUser();
        if(user && user->m_userId==uid && user->m_clientType==client)
            return it.second;
    }
    return nullptr;
}

void CImServer::kickUser(uint32_t uid, uint8_t client) {
    for(const auto& it:m_sessions){
        auto user = it.second->getUser();
        if(user && user->m_userId==uid && user->m_clientType==client){
            std::string blank;
            it.second->sendPacket(msg_type_kickuser, 0, blank);
        }
    }
}