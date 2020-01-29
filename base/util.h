#pragma once
#include <string>


std::string trim(const std::string& s);

void crash();


bool daemon_run();

pid_t getProcessId();

pthread_t getThreadid();

//可重入，返回当前时间的 tm 结构体和微妙
void getLocalTime(struct tm * pTm, int* usec);
