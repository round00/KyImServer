//
// Created by gjk on 2020/2/22.
//

#include "FileSession.h"
#include "FileMsg.h"
#include "Logger.h"
#include "ProtocolStream.h"
#include "FileManager.h"
#include <string.h>
#include <sys/stat.h>

void CFileSession::sendPacket(int32_t cmd, int32_t seq, int32_t errcode,
        const std::string &fmd5, int64_t offset, int64_t fsize, const std::string& data) {
    std::string packet;
    BinaryStreamWriter writer(&packet);
    writer.WriteInt32(cmd);
    writer.WriteInt32(seq);
    writer.WriteInt32(errcode);
    writer.WriteString(fmd5);
    writer.WriteInt64(offset);
    writer.WriteInt64(fsize);
    writer.WriteString(data);
    writer.Flush();

    std::string sendData;
    file_msg_header header;
    header.packagesize = packet.size();
    sendData.append((char*)&header, sizeof(header));
    sendData.append(packet);
    send(sendData);
}

void CFileSession::onMessage(CTcpConnection *conn) {
    while (true){
        int readable = conn->readableLength();
        if(conn->readableLength() < sizeof(file_msg_header)){
            return;
        }
        //提取包头
        file_msg_header header;
        conn->retriveBuffer((char*)&header, sizeof(header));
        if(header.packagesize<=0 || header.packagesize>=MAX_PACKET_SIZE){
            LOGE("packet size is invalid");
            return;
        }
        //提取包内容
        std::string packet;
        conn->retriveBuffer(packet, header.packagesize);
        //分发处理包
        if(!onPacketDispatch(packet)){
            LOGE("Packet dispatch failed");
            return ;
        }
    }
}

bool CFileSession::onPacketDispatch(std::string &packet) {
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
    std::string fmd5;
    size_t outlen;
    if(!reader.ReadString(&fmd5, 0, outlen)){
        LOGE("Read file md5 failed\n");
        return false;
    }
    int64_t offset;
    if(!reader.ReadInt64(offset)){
        LOGE("Read file offset failed\n");
        return false;
    }
    int64_t filesize;
    if(!reader.ReadInt64(filesize)){
        LOGE("Read file size failed\n");
        return false;
    }
    std::string filedata;
    if(!reader.ReadString(&filedata, 0, outlen)){
        LOGE("Read file data failed\n");
        return false;
    }
    switch (cmd){
        case msg_type_upload_req:
            onUploadFile(fmd5, offset, filesize, filedata);
            break;
        case msg_type_download_req:
            int32_t netType;
            if (!reader.ReadInt32(netType)){
                LOGE("read netType error");
                return false;
            }
            onDownloadFile(fmd5, netType);
            break;
        default:
            LOGI("recv unkown packet");
    }

    return true;
}

void CFileSession::onUploadFile(const std::string &fmd5, uint64_t offset,
        uint64_t fsize, std::string &data) {
    //文件已存在，并且文件指针也为空，说明这个文件已经上传过了
    if(FileManager::getInstance().fileExists(fmd5) && !m_pFile){
        std::string blank;
        sendPacket(msg_type_upload_resp, m_seq,file_msg_error_complete ,fmd5, fsize, fsize);
        return;
    }

    //如果传过来的offset大于0， 则说明是续传，此时session中应该保存着上传文件的指针
    if(offset>0){
        //文件指针不存在则无法续传
        if(!m_pFile){
            resetFileInfo();
            LOGE("upload file failed, because file pointer is null");
            return;
        }
    }else{
        std::string filename = FileManager::getInstance().getFileDir() + fmd5;
        m_pFile = fopen(filename.c_str(), "wb");
        if(!m_pFile){
            LOGE("upload file: create file failed, err=%s\", strerror(errno)");
            return;
        }
    }
    if(fseek(m_pFile, offset, SEEK_SET) != 0){
        LOGE("upload file: seek offset faild, err=%s", strerror(errno));
        return;
    }
    size_t nWrite = 0;
    if((nWrite = fwrite(data.c_str(), 1, data.length(), m_pFile)) != data.length()){
        LOGE("upload file: fwrite failed, ", data.length(), nWrite);
        return;
    }
    if(fflush(m_pFile) != 0){
        LOGE("upload file: fflush failed, err=%s", strerror(errno));
        return;
    }

    uint32_t errcode = file_msg_error_progress;
    offset += nWrite;
    if(offset == fsize){
        errcode = file_msg_error_complete;
        FileManager::getInstance().addFile(fmd5);
        resetFileInfo();
    }

    sendPacket(msg_type_upload_req, m_seq, errcode, fmd5, offset, fsize);
    LOGI("upload file: %s has uploaded %.2lf%%", fmd5.c_str(), (double)offset/fsize*100);
}

void CFileSession::onDownloadFile(const std::string &fmd5, int netType) {
    //文件步存在
    if(!FileManager::getInstance().fileExists(fmd5)){
        LOGI("download %s, this file not exists");
        sendPacket(msg_type_download_resp, m_seq, file_msg_error_not_exist, fmd5, 0, 0);
        return;
    }
    //打开文件
    if(!m_pFile){
        std::string filename = FileManager::getInstance().getFileDir() + fmd5;
        struct stat filestat;
        if(::stat(filename.c_str(), &filestat) != 0){
            LOGE("download: get file stat failed, err=%s", strerror(errno));
            return;
        }
        m_pFile = fopen(filename.c_str(), "rb+");
        if(!m_pFile){
            LOGE("download: open file failed, err=%s", strerror(errno));
            return;
        }
        m_downloadFileSize = filestat.st_size;
        m_downloadFileOffset = 0;
    }
    //文件大小比偏移量还小，那么肯定出错了
    if(m_downloadFileSize<=m_downloadFileOffset){
        LOGE("download: filesize less than offset");
        return;
    }

    //分成多块发送，每次发送数据的块大小
    int blockSize = 512*1024;
    if(netType == client_net_type_cellular){
        blockSize = 64*1024;
    }
    if(m_downloadFileSize < m_downloadFileOffset + blockSize){
        blockSize = m_downloadFileSize-m_downloadFileOffset;
    }
    char buf[512*1024];
    size_t nRead = 0;
    if((nRead = fread(buf, 1, blockSize, m_pFile)) == 0){
        LOGE("download: read file failed, err=%s", strerror(errno));
        return ;
    }
    int64_t offset = m_downloadFileOffset;
    m_downloadFileOffset += nRead;
    int errcode = file_msg_error_progress;
    if(m_downloadFileOffset == m_downloadFileSize){
        errcode = file_msg_error_complete;
        resetFileInfo();
    }

    sendPacket(msg_type_download_resp, m_seq, errcode, fmd5, offset,
            m_downloadFileSize, std::string(buf, nRead));
    LOGI("download: %s has download %2.lf%%", fmd5.c_str(),
            (double)m_downloadFileOffset/m_downloadFileSize*100);
}

void CFileSession::resetFileInfo() {
    if(m_pFile){
        fclose(m_pFile);
        m_pFile = nullptr;
    }
    m_downloadFileOffset = 0;
    m_downloadFileSize = 0;
}