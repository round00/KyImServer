//
// Created by gjk on 2020/2/22.
//

#ifndef KYIMSERVER_FILESESSION_H
#define KYIMSERVER_FILESESSION_H

#include "TcpSession.h"
class CFileSession : public CTcpSession{
public:
    explicit CFileSession(CTcpConnection* conn):
        CTcpSession(conn), m_seq(0){
        resetFileInfo();
    }

    void        sendPacket(int32_t cmd, int32_t seq, int32_t errcode,
            const std::string& fmd5, int64_t offset, int64_t fsize,
            const std::string& data = "");

    void        onMessage(CTcpConnection* conn);
    bool        onPacketDispatch(std::string& packet);

    void        onUploadFile(const std::string& fmd5, uint64_t offset, uint64_t fsize, std::string& data);
    void        onDownloadFile(const std::string& fmd5, int netType);
private:
    void        resetFileInfo();
    int32_t     m_seq;
    //文件指针，用于续传
    FILE*       m_pFile;
    //正在下载文件的大小
    uint64_t    m_downloadFileSize;
    //正在下载文件的偏移量
    uint64_t    m_downloadFileOffset;
};
#endif //KYIMSERVER_FILESESSION_H
