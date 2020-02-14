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
#include "UserManager.h"
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
//                case msg_type_operatefriend:
//                    onFriendOperator(data);
//                    break;
                case msg_type_userstatuschange:
                    onUserStateChange(data);
                    break;
                case msg_type_updateuserinfo:
                    onUpdateUserInfo(data);
                    break;
                case msg_type_modifypassword:
                    onModifyPassword(data);
                    break;
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

void CClientSession::sendPacketWithUserId(
        int32_t cmd, int32_t seq, std::string &data, uint32_t uid) {
    std::string packet;
    BinaryStreamWriter writer(&packet);
    writer.WriteInt32(cmd);
    writer.WriteInt32(seq);
    writer.WriteString(data);
    writer.WriteInt32(uid);
    writer.Flush();
    send(packet);
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
    if(UserManager::getInstance().addNewUser(user) == 0){//失败
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
    m_user = UserManager::getInstance().getUserByAccount(account);
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
}

void CClientSession::onGetFriendList(std::string &data) {
    //这个请求没有内容
    std::string response;
    Json::Value json;
    json["code"] = 0;
    json["msg"] = "ok";
    for(const auto& friendGroup:m_user->m_friendGroups){
        Json::Value fgroupInfo;
        fgroupInfo["teamname"] = friendGroup->getName();
        for(auto uid:friendGroup->getUserIds()){
            UserPtr fuser = UserManager::getInstance().getUserByUid(uid);
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
    response = Json::writeString(builder, json);
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
    UserPtr user = UserManager::getInstance().getUserByAccount(username);

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
    /*
    //type为1发出加好友申请 2 收到加好友请求(仅客户端使用) 3应答加好友 4删除好友请求 5应答删除好友
    //当type=3时，accept是必须字段，0对方拒绝，1对方接受
    cmd = 1005, seq = 0, {"userid": 9, "type": 1}
    cmd = 1005, seq = 0, {"userid": 9, "type": 2, "username": "xxx"}
    cmd = 1005, seq = 0, {"userid": 9, "type": 3, "username": "xxx", "accept": 1}

    //发送
    cmd = 1005, seq = 0, {"userid": 9, "type": 4}
    //应答
    cmd = 1005, seq = 0, {"userid": 9, "type": 5, "username": "xxx"}
 **/
//    Json::Reader reader;
//    Json::Value value;
//    if(!reader.parse(data, value)){
//        return;
//    }
//
//    std::string response;
//    Json::StreamWriterBuilder builder;
//    response = Json::writeString(builder, json);
//    sendPacket(msg_type_finduser, m_seq, response);
}

void CClientSession::onUserStateChange(std::string &data) {
    Json::Reader reader;
    Json::Value value;
    if(!reader.parse(data, value)){
        return;
    }

    int stateType = value["type"].asInt();
    int newStatus = value["onlinestatus"].asInt();
    m_user->m_onlineType = newStatus;
    Json::Value json;
    json["type"] = stateType;
    if(stateType==1){
        json["onlinestatus"] = newStatus;
        json["clienttype"] = m_user->m_clientType;
    }else if(stateType==2){
        json["onlinestatus"] = 0;
    }


    std::string response;
    Json::StreamWriterBuilder builder;
    response = Json::writeString(builder, json);
    //将状态改变的消息通知好友的各个客户端
    auto friends = UserManager::getInstance().getFriendListById(m_user->m_userId);
    for(auto id:friends){
        //可能存在多个客户端同时在线
        auto sessions = CImServer::getInstance().getClientSession(id);
        for(const auto& session:sessions){
            session->sendPacketWithUserId(msg_type_userstatuschange, m_seq, response, m_user->m_userId);
        }
    }

    sendPacket(msg_type_userstatuschange, m_seq, response);
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
    UserManager::getInstance().updateUserInfo(m_user->m_userId);

    //还利用发过来的json对象
    value["code"] = 0;
    value["msg"] = "ok";
    value["userid"] = m_user->m_userId;

    std::string response;
    Json::StreamWriterBuilder builder;
    response = Json::writeString(builder, value);

    sendPacket(msg_type_updateuserinfo, m_seq, response);

    //将状态改变的消息通知好友的各个客户端
    std::string stateChangeMsg = R"({"type":3})";
    auto friends = UserManager::getInstance().getFriendListById(m_user->m_userId);
    for(auto id:friends){
        //可能存在多个客户端同时在线
        auto sessions = CImServer::getInstance().getClientSession(id);
        for(const auto& session:sessions){
            session->sendPacketWithUserId(msg_type_userstatuschange, m_seq, stateChangeMsg, m_user->m_userId);
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
        UserManager::getInstance().updateUserPassowrd(m_user->m_userId);
    }

    sendPacket(msg_type_modifypassword, m_seq, response);
}