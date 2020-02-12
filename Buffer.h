//
// Created by gjk on 2020/2/11.
//

#ifndef KYIMSERVER_BUFFER_H
#define KYIMSERVER_BUFFER_H

#include <vector>
#include <string>

class CBuffer
{
public:
    CBuffer();
    //获取当前有效的buffer长度
    size_t              getLength(){return m_buffer.size()-m_start;}
    //添加buf到buffer中
    void                append(const char* buf, size_t len);
    //从buffer中提取内容
    size_t              retrive(char* buf, size_t n);
    size_t              retrive(std::string& buf, size_t n);

private:
    size_t              m_start;
    std::vector<char>   m_buffer;
};
#endif //KYIMSERVER_BUFFER_H
