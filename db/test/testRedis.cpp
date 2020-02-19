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
    redis->strSet("gjk", "haoshuai");
    string s = redis->strGet("gjk");
    string ss = redis->strGet("ytt");


    cout<<s<<endl;
    cout<<ss<<endl;
    cout<<'='<<endl;
    redis->changeDb(3);
    cout<<redis->strGet("ytt");
    return 0;
}