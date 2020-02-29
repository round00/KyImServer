//
// Created by gjk on 2020/2/22.
//

#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include "ConfigFile.h"
#include "Logger.h"
#include "FileManager.h"
#include "EventLoop.h"
#include "FileServer.h"
#include "util.h"

CEventLoop g_eventLoop;

int main(int argc, char* argv[])
{
    char op = -1;
    bool bDaemon = false;
    while((op = ::getopt(argc, argv, "d")) != -1){
        switch(op){
            case 'd':
                bDaemon = true;
                break;
        }
    }
    if(bDaemon){
        daemon_run();
    }

    //加载配置文件
    CConfigFile config("../../etc/fileserver.conf");
    //初始化日志
    std::string logdir = config.getValue("logdir");
    if(!CLogger::init("", LOG_LEVEL_DEBUG)){
        fprintf(stderr, "Logger initialize failed.\n");
        return 0;
    }
    std::string filedir = config.getValue("filedir");
    if(!FileManager::getInstance().init(filedir)){
        LOGE("FileManager init failed");
        return 0;
    }

    std::string serverPort = config.getValue("listenport");
    if(!CFileServer::getInstance().init(&g_eventLoop, atoi(serverPort.c_str()), 3)){
        LOGE("CFileServer initialize failed.");
        return 0;
    }

    LOGI("FileServer is ready, users can connected.");


    g_eventLoop.loop();
    return 0;
}