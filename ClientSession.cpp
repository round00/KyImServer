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
        default:
            LOGI("recv unkown packet");
            break;
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

