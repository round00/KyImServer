//
// Created by gjk on 2020/2/10.
//

#include "ClientSession.h"
#include "Msg.h"
#include "zlib/ZlibUtil.h"
#include "TcpServer.h"

void CClientSession::onMessage(CTcpConnection *conn) {
    fprintf(stderr, "%s called\n", __func__);
    while (true){
        fprintf(stderr, "readableLength = %d, %d\n", conn->readableLength(), sizeof(chat_msg_header));
        if(conn->readableLength() < sizeof(chat_msg_header)){
            return;
        }

        chat_msg_header header;
        conn->retriveBuffer((char*)&header, sizeof(header));
        fprintf(stderr, "compress=%d, ori length=%d, com length=%d\n",
                header.compressflag, header.originsize, header.compresssize);
        char buf[128];
        conn->retriveBuffer(buf, header.compresssize);
    }
}