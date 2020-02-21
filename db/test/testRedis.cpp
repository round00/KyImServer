//
// Created by gjk on 2020/1/30.
//

#include "../Redis.h"
#include <iostream>
using namespace std;


int main()
{
    RedisPtr redis = CRedis::connect("121.36.51.156", 8765, "nibuzhidao1");
    if(!redis)return 0;



    return 0;
}