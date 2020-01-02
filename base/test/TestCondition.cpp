//
// Created by gjk on 2020/1/2.
//
#include <stdio.h>
#include <queue>
#include <vector>
#include <unistd.h>
#include "../Mutex.h"
#include "../Condition.h"
#include "../Thread.h"

CMutex mutex;
CCondition cond;
std::queue<int> workqueue;

void* worker(void*){
    long tid = ::pthread_self();
    while(true){
        ::sleep(1);
        CMutexLock lock(mutex);
        while(workqueue.empty()){
            fprintf(stderr, "worker[%ld] waiting...\n", tid);
            cond.wait(mutex);
            fprintf(stderr, "worker[%ld] wakeup!\n", tid);
        }
        int worknumber = workqueue.front(); workqueue.pop();
        fprintf(stderr, "worker thread[%ld], get work[%d]\n", tid, worknumber);

    }
    return (void*)1;
}

int main()
{
    std::vector<CThread*> workers;
    //开几个工作线程
    for(int i = 0;i<3;++i){
        CThread *thread = new CThread(worker);
        workers.push_back(thread);
        thread->start();
    }
    int worknumber = 0;
    while(true){
        if(worknumber>20)break;
        //每秒派发5个任务
        sleep(1);
        CMutexLock lock(mutex);
        for(int i = 0;i<5;++i){
            workqueue.push(worknumber++);
        }
        cond.notifyAll();
    }
    return 0;
}