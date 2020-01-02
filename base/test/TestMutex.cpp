//
// Created by gjk on 2020/1/2.
//
#include <stdio.h>
#include <unistd.h>
#include "../Mutex.h"
#include "../Thread.h"

// 模拟死锁
CMutex mutex1, mutex2;
void*fun1(void* ){
    CMutexLock lock(mutex1);
    printf("%s get mutex1\n",__func__);
    sleep(2);
    CMutexLock lock1(mutex2);
    printf("%s get mutex2\n",__func__);

}
void* fun2(void*){
    CMutexLock lock(mutex2);
    printf("%s get mutex2\n",__func__);
    sleep(2);
    CMutexLock lock1(mutex1);
    printf("%s get mutex1\n",__func__);
}
int main()
{
//    CThread thread1(fun1), thread2(fun2);
//    thread1.start();
//    thread2.start();
//    sleep(10);
    CMutex mutex;
    {
        CMutexLock lock(mutex);
        int x= 1;
        x++;
        printf("%d\n",x );
    }

    return 0;
}

