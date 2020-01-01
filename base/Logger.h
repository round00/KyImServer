//
// Created by gjk on 2019/12/31.
//

#ifndef KYIMSERVER_LOGGER_H
#define KYIMSERVER_LOGGER_H

#include <string>
#include <queue>
#include "Mutex.h"
#include "Thread.h"
#include "Condition.h"

/*
 * 一些规则：
 * 1.程序启动的时候打开一个线程，专门用于写日志
 * 2.调用者设置日志目录和级别，实现负责生成文件。日志名：20191231_1627.log
 * 3.日志输出格式：[LEVEL][16:27:30.339][pid][tid][filename:lineno] logcontent
 * 4.每行日志增加最大长度限制，每个日志文件增加最大行数限制
 * 5.日志线程同时负责检查如果时间到了第二天或者超过了最大行数限制，则生成新的日志文件
 */

//低于设定level时不会输出日志
enum LOG_LEVEL{
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL     //FATAL级别的日志，写完后会挂掉程序
};

#define LOGD(...) CLogger::log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOGI(...) CLogger::log(LOG_LEVEL_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define LOGW(...) CLogger::log(LOG_LEVEL_WARNING, __FILE__, __LINE__, __VA_ARGS__)
#define LOGE(...) CLogger::log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define LOGF(...) CLogger::log(LOG_LEVEL_FATAL, __FILE__, __LINE__, __VA_ARGS__)

class CLogger
{
public:
    CLogger() = delete;
    ~CLogger() = delete;
public:
    static bool     init(const char* logFileName = "",
                LOG_LEVEL level = LOG_LEVEL_INFO,
                int maxRowLimit = 100000);
    static void     unint();
    static void     log(LOG_LEVEL level, const char* fileName, int lineNo, const char* format, ...);
    static FILE*    getLogFile();
    //返回距离上次检查时间的时间间隔
    static bool     getTimeGap();

    static void*    logThreadFunc(void*);
private:
    static bool             m_bInit;
    static std::string      m_strLogFileName;
    static FILE*            m_fLogFile;
    static LOG_LEVEL        m_nLogLevel;
    static int              m_nMaxRowLimit;     //单个日志文件最大行数限制
    static int              m_nCurRowNumber;    //当前日志文件里写了多少行了

    static CThread          m_logThread;        //写日志线程
    static std::queue<std::string>  m_logQueue; //待写日志队列
    static CMutex           m_logQueueMutex;    //待写日志队列锁
    static CCondition       m_logCondition;     //条件变量，同于通知有新日志要写

};

#endif //KYIMSERVER_LOGGER_H
