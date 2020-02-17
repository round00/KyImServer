//
// Created by gjk on 2020/2/10.
//

#include "ClientSession.h"
#include "Msg.h"
#include "zlib/ZlibUtil.h"
#include "TcpServer.h"
#include "Logger.h"
#include "ProtocolStream.h"
#include "jsoncpp/json.h"
#include "EntityManager.h"
#include "ImServer.h"
#include "FriendCroup.h"

void CClientSession::onMessage(CTcpConnection *conn) {
//    fprintf(stderr, "%s called\n", __func__);
    while (true){
//        fprintf(stderr, "readableLength = %d, %d\n", conn->readableLength(), sizeof(chat_msg_header));
        if(conn->readableLength() < sizeof(chat_msg_header)){
//            sendText("Invaild request");
            return;
        }
        //提取包头
        chat_msg_header header;
        conn->retriveBuffer((char*)&header, sizeof(header));
        fprintf(stderr, "compress=%d, ori length=%d, com length=%d\n",
                header.compressflag, header.originsize, header.compresssize);
        if(header.originsize<=0 || header.originsize>=MAX_PACKET_SIZE ||
            header.compresssize<=0 || header.compresssize>=MAX_PACKET_SIZE){
            LOGE("packet size is invalid");
            sendText("Invaild request");
            return;
        }
        //提取包内容
        std::string packet;
        if(header.compressflag==PACKAGE_COMPRESSED){
            std::string t;
            conn->retriveBuffer(t, header.compresssize);
            if(!Zlib::uncompressBuf(t, packet, header.originsize)){
                LOGE("Packet uncompressBuf failed");
                return;
            }
        }else{
            conn->retriveBuffer(packet, header.compresssize);
        }
        //分发处理包
        if(!onPacketDispatch(packet)){
            LOGE("Packet dispatch failed");
            return ;
        }
    }
}

bool CClientSession::onPacketDispatch(std::string &packet) {
    BinaryStreamReader reader(packet.c_str(), packet.length());
    int32_t cmd;
    if(!reader.ReadInt32(cmd)){
        LOGE("Read commond code failed\n");
        return false;
    }

    if(!reader.ReadInt32(m_seq)){
        LOGE("Read seq code failed\n");
        return false;
    }

    std::string data;
    size_t dataLength;
    if(!reader.ReadString(&data, 0, dataLength)){
        LOGE("Read packet data failed\n");
        return false;
    }

    LOGD("Recv Packet cmd=%d, seq=%d", cmd, m_seq);

    switch (cmd){
        case msg_type_heartbeat:
            onHeartBeat(data);
            break;
        case msg_type_register:
            onRegister(data);
            break;
        case msg_type_login:
            onLogin(data);
            break;
        default:{
            if(!m_bLogin){
                string response = R"({"code": 2, "msg": "not login, please login first!"})";
                sendPacket(cmd, m_seq, response);
                return true;
            }

            switch (cmd) {
                case msg_type_getfriendlist:
                    onGetFriendList(data);
                    break;
                case msg_type_finduser:
                    onFindUser(data);
                    break;
                case msg_type_operatefriend:
                    onFriendOperator(data);
                    break;
                case msg_type_userstatuschange:
                    onUserStateChange(data);
                    break;
                case msg_type_updateuserinfo:
                    onUpdateUserInfo(data);
                    break;
                case msg_type_modifypassword:
                    onModifyPassword(data);
                    break;
                case msg_type_creategroup:
                    onCreateGroup(data);
                    break;
                case msg_type_getgroupmembers:
                    onGetGroupMembers(data);
                    break;
                case msg_type_updatefriendgroup:
                {
                    int op;
                    size_t length;
                    reader.ReadInt32(op);
                    std::string newName;
                    reader.ReadString(&newName, 0, length);
                    std::string oldName;
                    reader.ReadString(&oldName, 0, length);
                    onUpdateFriendGroup(op, newName, oldName);
                    break;
                }
                case msg_type_movefriendgroup:
                {
                    int32_t fuid;
                    size_t length;
                    reader.ReadInt32(fuid);
                    std::string newName;
                    reader.ReadString(&newName, 0, length);
                    std::string oldName;
                    reader.ReadString(&oldName, 0, length);
                    onMoveFriendGroup(fuid, newName, oldName);
                    break;
                }
                case msg_type_chat:
                {
                    int32_t targetId;
                    reader.ReadInt32(targetId);
                    onChatMsg(data, targetId);
                    break;
                }
                default:
                    LOGI("recv unkown packet");
                    break;
            }
        }
    }

    return true;
}

void CClientSession::sendPacket(int32_t cmd, int32_t seq, std::string &data) {
    sendPacket(cmd, seq, data.c_str(), data.length());
}

void CClientSession::sendPacket(int32_t cmd, int32_t seq, const char *data, size_t dataLen) {
    std::string packet;
    BinaryStreamWriter writer(&packet);
    writer.WriteInt32(cmd);
    writer.WriteInt32(seq);
    writer.WriteCString(data, dataLen);
    writer.Flush();
    send(packet);
}

void CClientSession::send(const std::string &packet) {
    if(!m_conn){
        LOGE("Tcpconnection object is NULL");
        return;
    }

    std::string sendData;

    chat_msg_header header;
    header.originsize = packet.length();

    std::string compressPacket;
    if(Zlib::compressBuf(packet, compressPacket)){
        header.compressflag = PACKAGE_COMPRESSED;
        header.compresssize = compressPacket.length();
        sendData.append((char*)&header, sizeof(header));
        sendData.append(compressPacket);
    }else{
        header.compressflag = PACKAGE_UNCOMPRESSED;
        sendData.append((char*)&header, sizeof(header));
        sendData.append(packet);
    }

    m_conn->addWriteBuffer(sendData);
}

void CClientSession::sendText(const std::string &packet) {
    m_conn->addWriteBuffer(packet);
}


void CClientSession::makeInvalid() {
    m_user = nullptr;
    m_bLogin = false;
}

void CClientSession::onHeartBeat(std::string &data) {
    //心跳包返回空数据
    std::string s;
    sendPacket(msg_type_heartbeat, m_seq, s);
}

void CClientSession::onRegister(std::string &data) {
    Json::Reader reader;
    Json::Value value;
    if(!reader.parse(data, value)){
        return;
    }
    //{"username": "13917043329", "nickname": "balloon", "password": "123"}
    UserPtr user(new User());
    user->m_userAccount = value["username"].asString();
    user->m_userPhoneNum = value["username"].asString();
    user->m_nickName = value["nickname"].asString();
    user->m_userPassword = value["password"].asString();

    std::string response = R"({"code": 0, "msg": "ok"})";
    if(EntityManager::getInstance().addNewUser(user) == 0){//失败
        LOGI("User Register failed");
        response = R"({"code": 101, "msg": "failed"})";
    }

    sendPacket(msg_type_register, m_seq, response);
}

void CClientSession::onLogin(std::string &data) {
    Json::Reader reader;
    Json::Value value;
    if(!reader.parse(data, value)){
        return;
    }
    //{"username": "13917043329", "password": "123", "clienttype": 1, "status": 1}
    std::string account = value["username"].asString();
    m_user = EntityManager::getInstance().getUserByAccount(account);
    std::string response;
    if(!m_user){  //用户不存在
        response = R"({"code":102, "msg":"not registed"})";
    }else{
        if(m_user->m_userPassword != value["password"].asString()){//密码错误
            response = R"({"code":103, "msg":"incorrect password"})";
        }else {
            uint8_t clientType = value["clienttype"].asInt();
            auto session = CImServer::getInstance().getClientSession(m_user->m_userId, clientType);
            if(session && session.get() != this){   //session已经存在，并且不是当前session
                std::string blank;
                session->sendPacket(msg_type_kickuser, m_seq, blank);
                session->makeInvalid();
            }

            m_user->m_clientType = clientType;
            m_user->m_onlineType = value["status"].asInt();
            m_bLogin = true;

            Json::Value json;
            json["code"] = 0;
            json["msg"] = "ok";
            json["userid"] = m_user->m_userId;
            json["username"] = m_user->m_userAccount;
            json["nickname"] = m_user->m_nickName;
            json["phonenumber"] = m_user->m_userPhoneNum;
            //以下字段还未添加
            json["facetype"] = 0;
            json["gender"] = 0;
            json["birthday"] = 0;
            json["signature"] = "";
            json["address"] = "";
            json["customface"] = "";
            json["mail"] = "";
            Json::StreamWriterBuilder builder;
            response = Json::writeString(builder, json);
            LOGD("User %s(%d) login success", m_user->m_nickName.c_str(), m_user->m_userId);
        }
    }

    sendPacket(msg_type_login, m_seq, response);

    //获取离线消息
    auto offlineMsgs = EntityManager::getInstance().getOfflineMsgs(m_user->m_userId);
    for(const auto& msg:offlineMsgs){
        send(msg);
    }

    //通知好友我上线了
    auto friends = EntityManager::getInstance().getFriendListById(m_user->m_userId);
    for(auto friendId : friends){
        auto sessions = CImServer::getInstance().getClientSession(friendId);
        for(const auto& session:sessions){
            session->sendUserStateChangePacket(m_user->m_userId, 1);
        }
    }
}

void CClientSession::onGetFriendList(std::string &data) {
    //这个请求没有内容
    std::string response;
    makeFriendListPackge(response);
    sendPacket(msg_type_getfriendlist, m_seq, response);
}

void CClientSession::onFindUser(std::string &data) {
    Json::Reader reader;
    Json::Value value;
    if(!reader.parse(data, value)){
        return;
    }
    //type查找类型 0所有， 1查找用户 2查找群
    //{"type": 1, "username": "zhangyl"}
    //{ "code": 0, "msg": "ok", "userinfo": [{"userid": 2, "username": "qqq", "nickname": "qqq123", "facetype":0}] }
    std::string username = value["username"].asString();
    UserPtr user = EntityManager::getInstance().getUserByAccount(username);

    Json::Value json;
    if(user){
        json["code"] = 0;
        json["msg"] = "ok";
        Json::Value userinfo;
        userinfo["userid"] = user->m_userId;
        userinfo["username"] = user->m_userAccount;
        userinfo["nickname"] = user->m_nickName;
        userinfo["facetype"] = 0;
        json["userinfo"].append(userinfo);
    }else{  //查找失败返回好友列表空就行
        json["code"] = 0;
        json["msg"] = "ok";
        json["userinfo"].append(Json::Value());
    }

    std::string response;
    Json::StreamWriterBuilder builder;
    response = Json::writeString(builder, json);
    sendPacket(msg_type_finduser, m_seq, response);
}

void CClientSession::onFriendOperator(std::string &data) {
    Json::Reader reader;
    Json::Value value;
    if(!reader.parse(data, value)){
        return;
    }

    Json::Value retJson;
    std::string response;
    Json::StreamWriterBuilder builder;

    EntityManager& entityManager = EntityManager::getInstance();
    int opType = value["type"].asInt();
    uint32_t targetId = value["type"].asUInt();

    if(opType==1)   //添加
    {
        if(targetId<Group::GROUPID_BOUBDARY){ //好友
            if(!entityManager.isFriend(targetId, m_user->m_userId)){
                retJson["userid"] = m_user->m_userId;
                retJson["type"] = 2;
                retJson["username"] = m_user->m_nickName;
                response = Json::writeString(builder, retJson);

                auto sessions = CImServer::getInstance().getClientSession(targetId);
                if(sessions.empty()){
                    EntityManager::getInstance().addOfflineMsg(targetId, response);
                }else{
                    for(const auto& session : sessions){
                        session->sendPacket(msg_type_operatefriend, m_seq, response);
                    }
                }
            }
        }else{  //群组
            if(!entityManager.isGroupMember(targetId, m_user->m_userId)){
                //加群直接同意
                acceptJoinGroup(targetId);
            }
        }
    }
    else if(opType==2){    //仅客户端使用
    }
    else if(opType==3)  //应答添加
    {
        int accept = value["accept"].asInt();
        if(targetId<Group::GROUPID_BOUBDARY){
            answerMakeFriend(targetId, accept);
        }
    }
    else if(opType==4)  //删除
    {
        if(targetId<Group::GROUPID_BOUBDARY){
            deleteFriend(targetId);
        }else{
            quitGroup(targetId);
        }
    }
    else if(opType==5){    //应答删除近客户端使用
    }
}

void CClientSession::onUserStateChange(std::string &data) {
    Json::Reader reader;
    Json::Value value;
    if(!reader.parse(data, value)){
        return;
    }

//    int stateType = value["type"].asInt();
    int newStatus = value["onlinestatus"].asInt();
    m_user->m_onlineType = newStatus;


    //将状态改变的消息通知好友的各个客户端
    auto friends = EntityManager::getInstance().getFriendListById(m_user->m_userId);
    for(auto id:friends){
        //可能存在多个客户端同时在线
        auto sessions = CImServer::getInstance().getClientSession(id);
        for(const auto& session:sessions){
            session->sendUserStateChangePacket(m_user->m_userId, 1);
        }
    }

    sendUserStateChangePacket(m_user->m_userId, 1);
}

void CClientSession::onUpdateUserInfo(std::string &data) {
    Json::Reader reader;
    Json::Value value;
    if(!reader.parse(data, value)){
        return;
    }

    if(value["nickname"].isString())m_user->m_nickName = value["nickname"].asString();
    if(value["phonenumber"].isString())m_user->m_userPhoneNum = value["phonenumber"].asString();
    if(value["customface"].isString());
    if(value["gender"].isInt());
    if(value["birthday"].isInt());
    if(value["signature"].isString());
    if(value["address"].isString());
    if(value["mail"].isString());
    //更新用户信息
    EntityManager::getInstance().updateUserInfo(m_user->m_userId);

    //还利用发过来的json对象
    value["code"] = 0;
    value["msg"] = "ok";
    value["userid"] = m_user->m_userId;

    std::string response;
    Json::StreamWriterBuilder builder;
    response = Json::writeString(builder, value);

    sendPacket(msg_type_updateuserinfo, m_seq, response);

    //将状态改变的消息通知好友的各个客户端
    auto friends = EntityManager::getInstance().getFriendListById(m_user->m_userId);
    for(auto id:friends){
        //可能存在多个客户端同时在线
        auto sessions = CImServer::getInstance().getClientSession(id);
        for(const auto& session:sessions){
            session->sendUserStateChangePacket(m_user->m_userId, 3);
        }
    }
}

void CClientSession::onModifyPassword(std::string &data) {
    Json::Reader reader;
    Json::Value value;
    if(!reader.parse(data, value)){
        return;
    }

    std::string response;
    std::string oldpass = value["oldpassword"].asString();
    std::string newpass = value["newpassword"].asString();
    if(m_user->m_userPassword != oldpass){
        response = R"({"code":101, "msg":"failed"})";
    }else {
        response = R"({"code":0, "msg":"ok"})";
        m_user->m_userPassword = newpass;
        EntityManager::getInstance().updateUserPassowrd(m_user->m_userId);
    }

    sendPacket(msg_type_modifypassword, m_seq, response);
}

void CClientSession::onCreateGroup(std::string &data) {
    Json::Reader reader;
    Json::Value value;
    if(!reader.parse(data, value)){
        return;
    }

    std::string groupName = value["groupname"].asString();
    GroupPtr group(new Group(m_user->m_userId, groupName));
    //添加群组
    uint32_t gid = EntityManager::getInstance().addNewGroup(group);
    Json::Value res;
    if(gid==0){
        res["code"] = 106;
        res["msg"] = "create group error";
    }else{
        res["code"] = 0;
        res["msg"] = "ok";
        res["groupid"] = gid;
        res["groupname"] = group->m_groupName;
    }

    std::string response;
    Json::StreamWriterBuilder builder;
    response = Json::writeString(builder, value);
    sendPacket(msg_type_creategroup, m_seq, response);
}

void CClientSession::onGetGroupMembers(std::string &data) {
    Json::Reader reader;
    Json::Value value;
    if(!reader.parse(data, value)){
        return;
    }
    uint32_t gid = value["groupid"].asUInt();
    auto group = EntityManager::getInstance().getGroupByGid(gid);
    if(!group){
        return;
    }
    value["code"] = 0;
    value["msg"] = "ok";
    for(auto uid:group->m_groupMembers){
        auto user = EntityManager::getInstance().getUserByUid(uid);
        if(!user)continue;
        Json::Value userinfo;
        userinfo["userid"] = user->m_userId;
        userinfo["username"] = user->m_userAccount;
        userinfo["nickname"] = user->m_nickName;
        userinfo["facetype"] = 0;
        userinfo["customface"] = "";
        userinfo["status"] = user->m_onlineType;
        userinfo["clienttype"] = user->m_clientType;

        value["members"].append(userinfo);
    }

    std::string response;
    Json::StreamWriterBuilder builder;
    response = Json::writeString(builder, value);
    sendPacket(msg_type_getgroupmembers, m_seq, response);
}

void CClientSession::onUpdateFriendGroup(int op,
        const std::string &newName, const std::string &oldName) {
    if(op==0){ //新建好友分组
        if(!EntityManager::getInstance().addFriendGroup(m_user->m_userId, newName)){
            return;
        }
    }else if(op==1){    //删除好友分组
        if(!EntityManager::getInstance().delFriendGroup(m_user->m_userId, oldName)){
            return;
        }
    }else if(op==2){    //修改好友分组
        if(!EntityManager::getInstance().modifyFriendGroup(m_user->m_userId, newName, oldName)){
            return;
        }
    }

    std::string response;
    makeFriendListPackge(response);
    sendPacket(msg_type_getfriendlist, m_seq, response);
}

void CClientSession::onMoveFriendGroup(uint32_t fuid,
        const std::string &newName, const std::string &oldName) {
    uint32_t from = 0,to = 0;
    for(const auto& fg:m_user->m_friendGroups){
        if(fg->getName()==oldName)from = fg->getId();
        if(fg->getName()==newName)to = fg->getId();
    }
    if(!EntityManager::getInstance().moveUserToOtherFGroup(m_user->m_userId, fuid, from, to)){
        return;
    }

    std::string response;
    makeFriendListPackge(response);
    sendPacket(msg_type_getfriendlist, m_seq, response);
}

void CClientSession::onChatMsg(std::string &data, uint32_t targetId) {

    std::string msg;
    BinaryStreamWriter writeStream(&msg);
    writeStream.WriteInt32(msg_type_chat);
    writeStream.WriteInt32(m_seq);
    writeStream.WriteString(data);
    //消息发送者
    writeStream.WriteInt32(m_user->m_userId);
    //消息接受者
    writeStream.WriteInt32(targetId);
    writeStream.Flush();

    if(targetId<Group::GROUPID_BOUBDARY){   //单聊
        auto sessions = CImServer::getInstance().getClientSession(targetId);
        if(sessions.empty()){   //不在线，存为离线消息
            EntityManager::getInstance().addOfflineMsg(targetId, msg);
        }else{
            for(auto& session:sessions){
                if(session->m_bLogin){
                    session->send(msg);
                }
            }
        }

    }else{  //群聊
        auto members = EntityManager::getInstance().getGroupMembers(targetId);
        for(auto member:members){
            auto sessions = CImServer::getInstance().getClientSession(member);
            if(sessions.empty()){ //不在线，存为离线消息
                EntityManager::getInstance().addOfflineMsg(member, msg);
            }else{
                for(auto& session:sessions){
                    if(session->m_bLogin){
                        session->send(msg);
                    }
                }
            }
        }
    }
}

void CClientSession::makeFriendListPackge(std::string& packge) {
    Json::Value json;
    json["code"] = 0;
    json["msg"] = "ok";
    for(const auto& friendGroup:m_user->m_friendGroups){
        Json::Value fgroupInfo;
        fgroupInfo["teamname"] = friendGroup->getName();
        for(auto uid:friendGroup->getUserIds()){
            UserPtr fuser = EntityManager::getInstance().getUserByUid(uid);
            Json::Value userinfo;
            userinfo["userid"] = fuser->m_userId;
            userinfo["username"] = fuser->m_userAccount;
            userinfo["nickname"] = fuser->m_nickName;
            userinfo["facetype"] = 0;
            userinfo["customface"] = "";
            userinfo["gender"] = 1;
            userinfo["birthday"] = 19900101,
                    userinfo["signature"] = "";
            userinfo["address"] = "";
            userinfo["phonenumber"] = fuser->m_userPhoneNum;
            userinfo["mail"] = "";
            userinfo["clienttype"] = fuser->m_clientType;
            userinfo["status"] = fuser->m_onlineType;
            userinfo["markname"] = "";
            fgroupInfo["members"].append(userinfo);
        }
        json["userinfo"].append(fgroupInfo);
    }

    Json::StreamWriterBuilder builder;
    packge = Json::writeString(builder, json);
}

void CClientSession::sendUserStateChangePacket(uint32_t userId, int stateType) {
    Json::Value json;
    json["type"] = stateType;
    if(stateType==1){
        UserPtr user = EntityManager::getInstance().getUserByUid(userId);
        json["onlinestatus"] = user->m_onlineType;
        json["clienttype"] = user->m_clientType;
    }else if(stateType==2){
        json["onlinestatus"] = 0;
    }
    std::string response;
    Json::StreamWriterBuilder builder;
    response = Json::writeString(builder, json);

    std::string packet;
    BinaryStreamWriter writeStream(&packet);
    writeStream.WriteInt32(msg_type_userstatuschange);
    writeStream.WriteInt32(m_seq);
    writeStream.WriteString(response);
    writeStream.WriteInt32(userId);
    writeStream.Flush();

    send(packet);
}

void CClientSession::acceptJoinGroup(uint32_t groupId) {
    auto group = EntityManager::getInstance().getGroupByGid(groupId);
    if(!group){
        return;
    }

    if(!EntityManager::getInstance().joinGroup(groupId, m_user->m_userId)){
        return;
    }

    Json::Value retJson;
    retJson["userid"] = groupId;
    retJson["type"] = 3;    //添加应答
    retJson["username"] = group->m_groupName;
    retJson["accept"] = 3;  //不知道为什么是3

    std::string response;
    Json::StreamWriterBuilder builder;
    response = Json::writeString(builder, retJson);
    sendPacket(msg_type_operatefriend, m_seq, response);

    //通知群组成员
    auto gmembers = EntityManager::getInstance().getGroupMembers(groupId);
    for(auto member:gmembers){
        auto sessions = CImServer::getInstance().getClientSession(member);
        for(const auto& session:sessions){
            session->sendUserStateChangePacket(groupId, 3);
        }
    }
}

void CClientSession::quitGroup(uint32_t groupId) {
    if(!EntityManager::getInstance().quitGroup(groupId, m_user->m_userId)){
        return;
    }
    auto group = EntityManager::getInstance().getGroupByGid(groupId);
    if(!group){
        return;
    }
    //发消息给主动退群的用户
    Json::Value retJson;
    retJson["userid"] = groupId;
    retJson["type"] = 5;
    retJson["username"] = group->m_groupName;

    std::string response;
    Json::StreamWriterBuilder builder;
    response = Json::writeString(builder, retJson);
    sendPacket(msg_type_operatefriend, m_seq, response);

    //发送消息给群成员
    auto gmembers = EntityManager::getInstance().getGroupMembers(groupId);
    for(auto member:gmembers){
        auto sessions = CImServer::getInstance().getClientSession(member);
        for(const auto& session:sessions){
            session->sendUserStateChangePacket(groupId, 3);
        }
    }
}

void CClientSession::answerMakeFriend(uint32_t friendId, int accept) {
    auto user = EntityManager::getInstance().getUserByUid(friendId);
    if(!user){
        return;
    }

    //进行添加好友的操作
    if(accept==1){
        EntityManager::getInstance().makeFriendRelation(m_user->m_userId, friendId);
    }

    Json::Value retJson;
    //回答自己
    retJson["userid"] = friendId;
    retJson["type"] = 3;
    retJson["username"] = user->m_nickName;
    retJson["accept"] = accept;

    std::string response;
    Json::StreamWriterBuilder builder;
    response = Json::writeString(builder, retJson);
    sendPacket(msg_type_operatefriend, m_seq, response);

    //回答对方
    retJson["username"] = m_user->m_nickName;
    retJson["userid"] = m_user->m_userId;
    response = Json::writeString(builder, retJson);
    auto sessions = CImServer::getInstance().getClientSession(friendId);
    if(sessions.empty()){
        EntityManager::getInstance().addOfflineMsg(friendId, response);
    }else{
        for(const auto&session:sessions){
            session->sendPacket(msg_type_operatefriend, m_seq, response);
        }
    }


}

void CClientSession::deleteFriend(uint32_t friendId) {
    auto user = EntityManager::getInstance().getUserByUid(friendId);
    if(!user){
        return;
    }

    if(EntityManager::getInstance().breakFriendRelation(m_user->m_userId, friendId)){
        return;
    }

    Json::Value retJson;
    //发送给主动删除的一方
    retJson["userid"] = friendId;
    retJson["type"] = 5;
    retJson["username"] = user->m_nickName;

    std::string response;
    Json::StreamWriterBuilder builder;
    response = Json::writeString(builder, retJson);
    sendPacket(msg_type_operatefriend, m_seq, response);

    //回答对方
    retJson["username"] = m_user->m_nickName;
    retJson["userid"] = m_user->m_userId;
    response = Json::writeString(builder, retJson);
    auto sessions = CImServer::getInstance().getClientSession(friendId);
    for(const auto&session:sessions){
        session->sendPacket(msg_type_operatefriend, m_seq, response);
    }
}


