//
// Created by gjk on 2019/12/31.
//

#include "Thread.h"
#include <signal.h>
#include <errno.h>


CThread::CThread(ThreadFunc func)
    :m_function(func),m_thread(0)
{
}
CThread::~CThread()
{
    stop();
}

bool CThread::start()
{
    if(!m_function) //线程函数都还没初始化
    {
        return false;
    }

    pthread_attr_t attr;
    if(::pthread_attr_init(&attr))
    {
        return false;
    }
    if(::pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED))
    {
        return false;
    }

    return ::pthread_create(&m_thread, &attr, m_function, nullptr) == 0;
}

bool CThread::isRunning()
{
    return ::pthread_kill(m_thread, 0) == 0;
}

void CThread::stop()
{
    if(isRunning()){
        ::pthread_cancel(m_thread);
    }
}