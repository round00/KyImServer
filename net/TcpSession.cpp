//
// Created by gjk on 2020/2/22.
//

#include "TcpSession.h"
#include "ProtocolStream.h"
#include "../ImServer/Msg.h"
#include "zlib/ZlibUtil.h"



void CTcpSession::send(const std::string &packet) {
    if(!m_conn){
        return;
    }
    m_conn->addWriteBuffer(packet);
}



