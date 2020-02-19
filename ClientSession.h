//
// Created by gjk on 2020/2/10.
//

#ifndef KYIMSERVER_CLIENTSESSION_H
#define KYIMSERVER_CLIENTSESSION_H
#include <string>
#include "EntityManager.h"

class CTcpConnection;
class CClientSession{
public:
    explicit CClientSession(CTcpConnection* conn):
        m_conn(conn),m_seq(0),m_bLogin(false),m_user(nullptr){}

    void        onMessage(CTcpConnection* conn);
    bool        onPacketDispatch(std::string& packet);

    void        sendPacket(int32_t cmd, int32_t seq, std::string& data);
    void        sendPacket(int32_t cmd, int32_t seq, const char* data, size_t dataLen);
    void        send(const std::string& packet);
    void        sendText(const std::string& packet);
    //使当前Session失效
    void        makeInvalid();

    //业务处理
    void        onHeartBeat(std::string& data);
    void        onRegister(std::string& data);
    void        onLogin(std::string& data);
    void        onGetFriendList(std::string& data);
    void        onFindUser(std::string& data);
    void        onFriendOperator(std::string& data);
    void        onUserStateChange(std::string& data);
    void        onUpdateUserInfo(std::string& data);
    void        onModifyPassword(std::string& data);
    void        onCreateGroup(std::string& data);
    void        onGetGroupMembers(std::string& data);
    void        onUpdateFriendGroup(int op,
            const std::string& newName, const std::string& oldName);
    void        onMoveFriendGroup(uint32_t fuid,
            const std::string& newName, const std::string& oldName);
    void        onChatMsg(std::string& data, uint32_t targetId);
    void        onMultiChatMsg(std::string& data, std::string& targets);
    //==========================================
    bool        isLogined(){return m_bLogin;}
    UserPtr     getUser(){return m_user;}
    void        makeFriendListPackge(std::string& packge);
    void        sendUserStateChangePacket(uint32_t userId, int stateType);
    void        acceptJoinGroup(uint32_t groupId);
    void        quitGroup(uint32_t groupId);
    void        answerMakeFriend(uint32_t friendId, int accept);
    void        deleteFriend(uint32_t friendId);

private:
    CTcpConnection*     m_conn;
    int32_t             m_seq;
    bool                m_bLogin;
    UserPtr             m_user;
};

#endif //KYIMSERVER_CLIENTSESSION_H
