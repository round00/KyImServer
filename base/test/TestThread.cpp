//
// Created by gjk on 2020/1/2.
//
#include <stdio.h>
#include <unistd.h>
#include "../Thread.h"

void* func(void*){
    for(int i = 0;i<10;++i) {
        printf("second %d\n", i);
        sleep(1);
    }
    return (void*)1;
}
int main()
{
    CThread thread(func);
    if(!thread.start()){
        printf("thread start failed\n");
        return 0;
    }
    sleep(2);
    for(int i = 0;i<5; ++i){
        bool ret = thread.isRunning();
        printf("thread is alive=%d\n", ret);
        sleep(1);
    }
    thread.stop();
    puts("-------------------------");
    bool ret = thread.isRunning();
    printf("thread is alive=%d\n", ret);
    sleep(1);
    ret = thread.isRunning();
    printf("thread is alive=%d\n", ret);
    return 0;
}
