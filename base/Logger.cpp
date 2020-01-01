//
// Created by gjk on 2019/12/31.
//

#include "Logger.h"
#include "util.h"

bool CLogger::m_bInit = false;
std::string CLogger::m_strLogFileName;
FILE* CLogger::m_fLogFile = nullptr;
LOG_LEVEL CLogger::m_nLogLevel;
int CLogger::m_nMaxRowLimit;
int CLogger::m_nCurRowNumber = 0;

CThread CLogger::m_logThread(logThreadFunc);
std::queue<std::string>  CLogger::m_logQueue;
CMutex CLogger::m_logQueueMutex;
CCondition CLogger::m_logCondition;

bool CLogger::init(const char *logFileName, LOG_LEVEL level, int maxRowLimit) {
    unint();
    if(!logFileName || !logFileName[0])
    {
        m_strLogFileName = "";
    }
    else
    {
        m_strLogFileName = logFileName;
    }

    m_nLogLevel = level;
    m_nMaxRowLimit = maxRowLimit;
    m_nCurRowNumber = 0;

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
        m_logThread.stop();
    }

    if(m_fLogFile){
        fclose(m_fLogFile);
        m_fLogFile = nullptr;
    }
    //清空日志输出队列
    while(!m_logQueue.empty())m_logQueue.pop();
    m_strLogFileName.clear();
}

void* CLogger::logThreadFunc(void *)
{

    return (void*)1;
}