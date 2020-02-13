#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include "ConfigFile.h"
#include "Logger.h"
#include "UserManager.h"
#include "EventLoop.h"
#include "ImServer.h"

using std::cout;
using std::endl;

CEventLoop g_eventLoop;

int main()
{
    //加载配置文件
	CConfigFile config("../etc/chatserver.conf");
	//初始化日志
    std::string logdir = config.getValue("logdir");
    if(!CLogger::init("", LOG_LEVEL_DEBUG)){
        fprintf(stderr, "Logger initialize failed.\n");
        return 0;
    }
    //读取redis配置
	std::string redishost = config.getValue("redisserver");
	std::string port = config.getValue("redisport");
	std::string pass = config.getValue("redispass");
    //初始化用户管理器
    int redisport = atoi(port.c_str());
    if(!UserManager::getInstance().init(redishost, redisport, pass)){
        LOGE("UserManager initialize failed.");
        return 0;
    }

    std::string imServerPort = config.getValue("listenport");
    if(!CImServer::getInstance().init(&g_eventLoop, atoi(imServerPort.c_str()), 3)){
        LOGE("CImServer initialize failed.");
        return 0;
    }

    LOGI("KyImServer is ready, users can connected.");


    g_eventLoop.loop();
	return 0;
}