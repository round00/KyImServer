//
// Created by gjk on 2020/2/10.
//

#include "ImServer.h"
#include "ClientSession.h"
#include "Msg.h"
#include "Logger.h"

bool CImServer::init(CEventLoop *loop, int listenport, int thread) {
    setConnectCallBack(std::bind(&CImServer::onConnected, this, std::placeholders::_1));
    setCloseCallBack(std::bind(&CImServer::onClosed, this, std::placeholders::_1));
    return CTcpServer::init(loop, listenport, thread);
}

void CImServer::onConnected(CTcpConnection *conn) {
//    fprintf(stderr, "new Connection from %d\n", conn->m_clientFd);
    std::shared_ptr<CClientSession> session(new CClientSession(conn));
    conn->setMessageCallBack(std::bind(&CClientSession::onMessage, session.get(), std::placeholders::_1));

    CMutexLock lock(m_mutex);
    m_sessions[conn->m_clientFd] = session;
    LOGD("The number of online connections is %d", m_sessions.size());
}

void CImServer::onClosed(CTcpConnection *conn) {
    UserPtr user = nullptr;
    {//删除这个连接会话
        CMutexLock lock(m_mutex);
        auto it = m_sessions.find(conn->m_clientFd);
        if(it != m_sessions.end()) {
            user = it->second->getUser();
            m_sessions.erase(it);
        }
    }

    if(user){
        user->m_onlineType = 0;
        user->m_clientType = 0;
        //断开的连接上如果有用户登录的话，通知其好友下线
        auto friends = EntityManager::getInstance().getFriendListById(user->m_userId);
        for(auto fuser:friends){
            auto sessions = getClientSession(fuser);
            for(const auto& session: sessions){
                session->sendUserStateChangePacket(user->m_userId, 2);
            }
        }
        LOGD("User %s(client:%d) disconnected",user->m_userAccount.c_str(), user->m_clientType);
    }

    LOGD("The number of online connections is %d", m_sessions.size());
}

std::vector<std::shared_ptr<CClientSession>>
    CImServer::getClientSession(uint32_t uid) {
    std::vector<std::shared_ptr<CClientSession>> ret;

    CMutexLock lock(m_mutex);
    for(const auto& it:m_sessions){
        auto user = it.second->getUser();
        if(user && user->m_userId==uid){
            ret.push_back(it.second);
        }
    }
    return ret;
}

std::shared_ptr<CClientSession>
        CImServer::getClientSession(uint32_t uid, uint8_t client) {
    CMutexLock lock(m_mutex);
    for(const auto& it:m_sessions){
        auto user = it.second->getUser();
        if(user && user->m_userId==uid && user->m_clientType==client)
            return it.second;
    }
    return nullptr;
}

void CImServer::kickUser(uint32_t uid, uint8_t client) {
    CMutexLock lock(m_mutex);
    for(const auto& it:m_sessions){
        auto user = it.second->getUser();
        if(user && user->m_userId==uid && user->m_clientType==client){
            std::string blank;
            it.second->sendPacket(msg_type_kickuser, 0, blank);
            break;
        }
    }
}