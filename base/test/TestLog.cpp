//
// Created by gjk on 2020/1/9.
//

#include "../Logger.h"
#include <stdlib.h>
#include "../util.h"
#include <unistd.h>

//随机生成写入内容，随机长度，随机字符

std::string getRandomContent(){
    int len = rand()%100;
    std::string ans;
    for(int i = 0; i<len; ++i){
        char c = rand()%('z'-'a') + 'a';
        ans.append(1, c);
    }
    return ans;
}
//测试不设置日志目录的情况，写到控制台
void testNoLogDir(){
    ::srand(1024);
    CLogger::init("", LOG_LEVEL_DEBUG);
    //写1000条日志
    for(int i = 0;i<1000; ++i){
        LOGD("%s", getRandomContent().c_str());
    }
}
//测试指定日志目录的情况，写到该文件
void testLogDir(const char* logdir){
    ::srand(1024);
    CLogger::init(logdir, LOG_LEVEL_DEBUG);
    LOGD("%s", getRandomContent().c_str());
    //写1000条日志
    for(int i = 0;i<100; ++i){
        LOGE("%s", getRandomContent().c_str());
    }
}
int main()
{
    testLogDir("/tmp/logs/");
    sleep(5);
    return 0;
}
