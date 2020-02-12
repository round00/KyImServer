//
// Created by gjk on 2020/2/11.
//
#include <md5.h>
#include "Buffer.h"

CBuffer::CBuffer():m_start(0){

}

void CBuffer::append(const char *buf, size_t len) {
    for(int i = 0;i<len;++i)
        m_buffer.push_back(buf[i]);
}

size_t CBuffer::retrive(char *buf, size_t n) {
    size_t retriveLen = getLength();
    //当前有足够的内容供提取
    if(retriveLen>n){
        retriveLen = n;
    }
    //没有东西可提取了
    if(retriveLen==0){
        return 0;
    }

    memcpy(buf, &m_buffer[m_start], retriveLen);
    m_start += retriveLen;
    return retriveLen;
}

size_t CBuffer::retrive(std::string &buf, size_t n) {
    size_t retriveLen = getLength();
    //当前有足够的内容供提取
    if(retriveLen>n){
        retriveLen = n;
    }
    //没有东西可提取了
    if(retriveLen==0){
        return 0;
    }

    buf.append(&m_buffer[m_start], retriveLen);
    m_start += retriveLen;
    return retriveLen;
}