//
// Created by gjk on 2019/12/31.
//

#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#include "Logger.h"
#include "util.h"
#include "Mutex.h"
#include "Thread.h"
#include "Condition.h"

bool CLogger::m_bInit = false;
std::string CLogger::m_strLogDir;
FILE* CLogger::m_fLogFile = nullptr;
LOG_LEVEL CLogger::m_nLogLevel;
int CLogger::m_nLastCheckDay;


bool CLogger::m_bRunning = false;
CThread CLogger::m_logThread(logThreadFunc);
std::queue<std::string>  CLogger::m_logQueue;
CMutex CLogger::m_mutexLogQueue;
CCondition CLogger::m_condLogQueue;

bool CLogger::init(const char *logDir, LOG_LEVEL level, int maxRowLimit) {
    unint();
    if(!logDir || !logDir[0])
    {
        m_strLogDir = "";
    }
    else
    {
        m_strLogDir = logDir;
    }

    m_nLogLevel = level;

    m_bRunning = true;
    if(!m_logThread.start())
    {
        return false;
    }
    m_bInit = true;
    return true;
}

void CLogger::unint() {
    m_bInit = false;
    if(m_logThread.isRunning()){
        m_bRunning = false;
        m_logThread.stop();
    }

    if(m_fLogFile){
        ::fclose(m_fLogFile);
        m_fLogFile = nullptr;
    }
    //清空日志输出队列
    while(!m_logQueue.empty())m_logQueue.pop();
    m_strLogDir.clear();
}

void CLogger::log(LOG_LEVEL level, const char *fileName, int lineNo, const char *format, ...)
{
    //还没有初始化好，不能写入日志
    if(!m_bInit)return;

}
std::string CLogger::getLogFileName()
{
    //设置的日志目录为空
    if(m_strLogDir.empty())
    {
        return "";
    }
    //确定目录是否存在，不存在则创建
    if(::access(m_strLogDir.c_str(), F_OK) != 0)
    {
        if(::mkdir(m_strLogDir.c_str(), 0777)!=0)
        {
            crash();
        }
    }

    struct tm* now = localtime(NULL);
    char logFileName[64];
    snprintf(logFileName, 64,"%4d%02d%02d_%02d.log", now->tm_year, now->tm_mon, now->tm_mday, now->tm_hour);
    delete now;
    std::string fullName = m_strLogDir + "/" + logFileName;

    return fullName;
}
FILE* CLogger::getLogFile()
{
    FILE * f = nullptr;
    std::string fileName = getLogFileName();
    if(fileName.empty())
    {
        f = stdout;
    }
    else
    {
        //文件不存在会创建
        f = ::fopen(fileName.c_str(), "a+");
        if(!f)
        {
            crash();
        }
    }
    return f;
}

void CLogger::writeToFile(const std::string &s)
{
    int n = s.length(), hasWritten = 0;
    while(hasWritten<n)
    {
        int nWrite = ::fwrite(s.c_str() + hasWritten, 1, n-hasWritten, m_fLogFile);
        hasWritten += nWrite;
    }
}

bool CLogger::checkGapADay()
{
    struct tm* now = localtime(NULL);
    int curDay = now->tm_mday;
    delete now;

    return curDay != m_nLastCheckDay;
}

void* CLogger::logThreadFunc(void *)
{
    while(m_bRunning){
        //获取到日志队列的访问权限后，先把日志队列的内容放到一个临时队列里，让队列锁抓紧释放
        std::queue<std::string> logQueue;
        {//等待读取日志待写队列
            CMutexLock lock(m_mutexLogQueue);
            while(m_logQueue.empty()){
                m_condLogQueue.wait(m_mutexLogQueue);
            }
            logQueue.swap(m_logQueue);
        }

        if(!m_fLogFile || checkGapADay()){
            m_fLogFile = getLogFile();
        }

        while(!logQueue.empty()){
            std::string strLog = logQueue.front();
            logQueue.pop();
            writeToFile(strLog);
        }
    }
    return (void*)1;
}