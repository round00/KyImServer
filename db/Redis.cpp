//
// Created by gjk on 2020/1/30.
//

#include "Redis.h"
#include <hiredis/hiredis.h>
#include <cstring>

RedisPtr CRedis::connect(const string& ip, int port, const string& passwd){
    RedisPtr redis(new CRedis(ip, port, passwd));
    if(!redis->init()){
        return nullptr;
    }

    return redis;
}

bool CRedis::init() {
    m_conn.reset(::redisConnect(m_hostIP.c_str(), m_port), redisFree);
    if(m_conn->err){
        fprintf(stderr, "connect to redis failed,err=%s\n", m_conn->errstr);
        return false;
    }
    //如果有设置密码的话，验证密码
    if(!m_password.empty()){
        auto reply = sendACommand("auth", m_password);
        if(reply->type == REDIS_REPLY_ERROR){
            fprintf(stderr, "auth failed, err=%s\n", reply->str);
            return false;
        }
    }

    //连接成功先ping测试一下是否通
    if(!isConnected()){
        fprintf(stderr, "connected test failed\n");
        return false;
    }

    return true;
}

RedisReply CRedis::sendACommand(int argc, const char **argv, const size_t *argvlen) {
    RedisReply reply(static_cast<redisReply*>(
                             ::redisCommandArgv(m_conn.get(), argc, argv, argvlen)));
    if(reply->type==REDIS_REPLY_ERROR){
        if(m_conn->err==REDIS_ERR_IO){
            fprintf(stderr, "redis command failed, err=%s\n", strerror(errno));
        }else {
            fprintf(stderr, "redis command failed, err=%s\n", m_conn->errstr);
        }
        return nullptr;
    }
    return reply;
}

RedisReply CRedis::sendACommand(const string &cmd, const string& key) {
    //构造参数列表
    const char* argv[2];
    size_t argvlen[2];
    argv[0] = cmd.c_str();
    argvlen[0] = cmd.length();
    argv[1] = key.c_str();
    argvlen[1] = key.length();

    return sendACommand(2, argv, argvlen);
}

RedisReply CRedis::sendACommand(const string &cmd, const string &key, const string &val) {
    //构造参数列表
    const char* argv[3];
    size_t argvlen[3];
    argv[0] = cmd.c_str();
    argvlen[0] = cmd.length();
    argv[1] = key.c_str();
    argvlen[1] = key.length();
    argv[2] = val.c_str();
    argvlen[2] = val.length();

    return sendACommand(3, argv, argvlen);
}

RedisReply CRedis::sendACommand(const string &cmd, const std::vector<string> &params) {
    if(params.size()>=MAX_PARAMS){
        fprintf(stderr, "redis command failed, err=too many params\n");
        return nullptr;
    }
    //构造参数列表
    const char* argv[MAX_PARAMS+1];
    size_t argvlen[MAX_PARAMS+1];
    argv[0] = cmd.c_str();
    argvlen[0] = cmd.length();
    int argc = params.size() + 1;

    for(size_t i = 0;i<params.size(); ++i){
        argv[i+1] = params[i].c_str();
        argvlen[i+1] = params[i].length();
    }

    return sendACommand(argc, argv, argvlen);
}

RedisReply CRedis::sendACommand(const string &cmd, const string &key,
        const std::vector<string> &params) {
    if(params.size()>=MAX_PARAMS){
        fprintf(stderr, "redis command failed, err=too many params\n");
        return nullptr;
    }
    //构造参数列表
    const char* argv[MAX_PARAMS+2];
    size_t argvlen[MAX_PARAMS+2];
    argv[0] = cmd.c_str();
    argvlen[0] = cmd.length();
    argv[1] = key.c_str();
    argvlen[1] = key.length();
    int argc = params.size() + 2;

    for(size_t i = 0;i<params.size(); ++i){
        argv[i+2] = params[i].c_str();
        argvlen[i+2] = params[i].length();
    }

    return sendACommand(argc, argv, argvlen);
}

bool CRedis::isConnected() {
    RedisReply reply = sendACommand("ping", std::vector<string>());
    return reply && reply->type==REDIS_REPLY_STATUS
        && strcmp(reply->str, "PONG")==0;
}

bool CRedis::changeDb(int db) {
    RedisReply rep = sendACommand("select", std::to_string(db));
    return rep && rep->type==REDIS_REPLY_STATUS
        &&strcmp(rep->str, "OK")==0;
}

bool CRedis::deleteKey(const string &key) {
    RedisReply rep = sendACommand("del", key);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return false;
    }
    return rep->integer>=0;
}

//=========================字符串相关命令=========================
bool CRedis::strSet(const string &key, const string &value) {
    RedisReply rep = sendACommand("set", key, value);
    return rep && rep->type==REDIS_REPLY_STATUS
           &&strcmp(rep->str, "OK")==0;
}

string CRedis::strGet(const string &key) {
    RedisReply reply = sendACommand("get", key);
    if(!reply || reply->type==REDIS_REPLY_NIL || !reply->str){
        return "";
    }
    return reply->str;
}

LL CRedis::strGetNumValue(const string &key) {
    auto s = strGet(key);
    if(s.empty())return INT_NULL;
    return atoll(s.c_str());
}

LL CRedis::strIncr(const string &key) {
    RedisReply rep = sendACommand("incr", key);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return -1;
    }
    return rep->integer;
}

LL CRedis::strIncrBy(const string &key, LL by) {
    RedisReply rep = sendACommand("incr", key, std::to_string(by));
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return -1;
    }
    return rep->integer;
}

LL CRedis::strDecr(const string &key) {
    RedisReply rep = sendACommand("decr", key);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return -1;
    }
    return rep->integer;
}

LL CRedis::strDecrBy(const string &key, LL by) {
    RedisReply rep = sendACommand("decr", key, std::to_string(by));
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return -1;
    }
    return rep->integer;
}

//=========================列表相关命令=========================
int CRedis::listLpush(const string &key, int val) {
    return listLpush(key, std::to_string(val));
}

int CRedis::listLpush(const string &key, const string &val) {
    RedisReply rep = sendACommand("lpsuh", key, val);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return 0;
    }
    return rep->integer;
}

int CRedis::listLpush(const string &key, const std::vector<string> &val) {
    if(val.empty())return 0;
    RedisReply rep = sendACommand("lpush", key, val);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return 0;
    }
    return rep->integer;
}

int CRedis::listRpush(const string &key, int val) {
    return listRpush(key, std::to_string(val));
}

int CRedis::listRpush(const string &key, const string &val) {
    RedisReply rep = sendACommand("rpsuh", key, val);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return 0;
    }
    return rep->integer;
}

int CRedis::listRpush(const string &key, const std::vector<string> &val) {
    if(val.empty())return 0;
    RedisReply rep = sendACommand("rpush", key, val);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return 0;
    }
    return rep->integer;
}

string CRedis::listLpop(const string &key) {
    RedisReply rep = sendACommand("lpop", key);
    if(!rep || rep->type!=REDIS_REPLY_STRING){
        return "";
    }
    return rep->str;
}

string CRedis::listRpop(const string &key) {
    RedisReply rep = sendACommand("rpop", key);
    if(!rep || rep->type!=REDIS_REPLY_STRING){
        return "";
    }
    return rep->str;
}

int CRedis::listDelete(const string &key, const string &val) {
    std::vector<string> params;
    params.push_back(key);
    params.push_back("0");
    params.push_back(val);
    RedisReply rep = sendACommand("lrem", params);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return 0;
    }
    return rep->integer;
}

int CRedis::listLength(const string &key) {
    RedisReply rep = sendACommand("llen", key);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return 0;
    }
    return rep->integer;
}


string CRedis::listGetByIndex(const string &key, int index) {
    RedisReply rep = sendACommand("lindex", key, std::to_string(index));
    if(!rep || rep->type!=REDIS_REPLY_STRING){
        return "";
    }
    return rep->str;
}

std::vector<string> CRedis::listGetByRange(const string &key, int start, int stop) {
    std::vector<string> res, params={key, std::to_string(start), std::to_string(stop)};
    RedisReply rep = sendACommand("lrange", params);
    if(!rep || rep->type!=REDIS_REPLY_ARRAY){
        return res;
    }
    for(size_t i = 0; i<rep->elements; ++i){
        res.emplace_back(rep->element[i]->str);
    }

    return res;
}

//=========================哈希表相关命令=========================
bool CRedis::hashSetKeyValue(const string &hash, const string &key, const string &val) {
    std::vector<string> params = {hash, key, val};
    RedisReply rep = sendACommand("hset", params);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return false;
    }
    return rep->integer>=0;
}

bool CRedis::hashSetKeyValues(const string &hash,
        const std::vector<std::pair<string, string>>& keyvals) {
    std::vector<string> params;
    for(const auto& p:keyvals){
        params.push_back(p.first);
        params.push_back(p.second);
    }
    RedisReply rep = sendACommand("hmset", hash, params);
    if(!rep || rep->type!=REDIS_REPLY_STATUS){
        return false;
    }
    return strcmp(rep->str, "OK")==0;

}

string CRedis::hashGetValue(const string &hash, const string &key) {
    RedisReply rep = sendACommand("hget", hash, key);
    if(!rep || rep->type!=REDIS_REPLY_STRING){
        return "";
    }
    return rep->str;
}

bool CRedis::hashExistKey(const string &hash, const string &key) {
    RedisReply rep = sendACommand("hexists", hash, key);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return false;
    }
    return rep->integer==1;
}

int CRedis::hashDeleteKey(const string &hash, const string &key) {
    RedisReply rep = sendACommand("hdel", hash, key);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return 0;
    }
    return rep->integer;
}

int CRedis::hashDeleteKeys(const string &hash, const std::vector<string> &keys) {
    RedisReply rep = sendACommand("hdel", hash, keys);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return 0;
    }
    return rep->integer;
}

int CRedis::hashLength(const string &hash) {
    RedisReply rep = sendACommand("hlen", hash);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return 0;
    }
    return rep->integer;
}

int CRedis::hashAddByInt(const string &hash, const string &key, int by) {
    std::vector<string> params = {hash, key, std::to_string(by)};
    RedisReply rep = sendACommand("hincrby", params);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return INT_NULL;
    }
    return rep->integer;
}

std::vector<string> CRedis::hashGetKeys(const string &hash) {
    std::vector<string> res;
    RedisReply rep = sendACommand("hkeys", hash);
    if(!rep || rep->type!=REDIS_REPLY_ARRAY){
        return res;
    }

    for(size_t i = 0; i<rep->elements; ++i){
        res.emplace_back(rep->element[i]->str);
    }
    return res;
}
std::vector<string> CRedis::hashGetValues(const string &hash) {
    std::vector<string> res;
    RedisReply rep = sendACommand("hvals", hash);
    if(!rep || rep->type!=REDIS_REPLY_ARRAY){
        return res;
    }

    for(size_t i = 0; i<rep->elements; ++i){
        res.emplace_back(rep->element[i]->str);
    }
    return res;
}

std::vector<std::pair<string,string>> CRedis::hashGetKeyValues(const string &hash) {
    std::vector<std::pair<string,string>> res;
    RedisReply rep = sendACommand("hgetall", hash);
    if(!rep || rep->type!=REDIS_REPLY_ARRAY){
        return res;
    }

    for(size_t i = 0; i<rep->elements; i+=2){
        res.emplace_back(std::make_pair(
                rep->element[i]->str, rep->element[i+1]->str));
    }
    return res;
}

//=========================集合相关命令=========================
int CRedis::setAddKey(const string &set, const string &key) {
    RedisReply rep = sendACommand("sadd", set, key);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return 0;
    }
    return rep->integer;
}

int CRedis::setAddKeys(const string &set, const std::vector<string> &keys) {
    RedisReply rep = sendACommand("sadd", set, keys);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return 0;
    }
    return rep->integer;
}

bool CRedis::setExists(const string &set, const string &key) {
    RedisReply rep = sendACommand("sismember", set, key);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return false;
    }
    return rep->integer==1;
}

string CRedis::setRandomDelete(const string &set) {
    RedisReply rep = sendACommand("spop", set);
    if(!rep || rep->type!=REDIS_REPLY_STRING){
        return "";
    }
    return rep->str;
}

string CRedis::setRandomGet(const string &set) {
    RedisReply rep = sendACommand("srandmember", set);
    if(!rep || rep->type!=REDIS_REPLY_STRING){
        return "";
    }
    return rep->str;
}

int CRedis::setDeleteKey(const string &set, const string &key) {
    RedisReply rep = sendACommand("srem", set, key);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return false;
    }
    return rep->integer;
}

int CRedis::setDeleteKeys(const string &set, const std::vector<string> &keys) {
    string cmd = "srem " + set;
    for(const string& key:keys){
        cmd += " " + key;
    }
    RedisReply rep = sendACommand("srem", set, keys);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return false;
    }
    return rep->integer;
}

bool CRedis::setMoveKey(const string &from, const string &to, const string &key) {
    std::vector<string> params = {from, to, key};
    RedisReply rep = sendACommand("smove", params);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return false;
    }
    return rep->integer==1;
}

int CRedis::setLength(const string &set) {
    RedisReply rep = sendACommand("scard", set);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return 0;
    }
    return rep->integer;
}

std::vector<string> CRedis::setGetKeys(const string &set) {
    std::vector<string> res;
    RedisReply rep = sendACommand("smembers", set);
    if(!rep || rep->type!=REDIS_REPLY_ARRAY){
        return res;
    }
    for(size_t i = 0;i<rep->elements; ++i){
        res.emplace_back(rep->element[i]->str);
    }
    return res;
}

std::vector<string> CRedis::setIntersection(const std::vector<string>& sets) {
    std::vector<string> res;
    RedisReply rep = sendACommand("sinter", sets);
    if(!rep || rep->type!=REDIS_REPLY_ARRAY){
        return res;
    }
    for(size_t i = 0;i<rep->elements; ++i){
        res.emplace_back(rep->element[i]->str);
    }
    return res;
}

std::vector<string> CRedis::setUnion(const std::vector<string>& sets) {
    std::vector<string> res;
    RedisReply rep = sendACommand("sunion", sets);
    if(!rep || rep->type!=REDIS_REPLY_ARRAY){
        return res;
    }
    for(size_t i = 0;i<rep->elements; ++i){
        res.emplace_back(rep->element[i]->str);
    }
    return res;
}

std::vector<string> CRedis::setDifference(const std::vector<string>& sets) {
    std::vector<string> res;
    RedisReply rep = sendACommand("sdiff", sets);
    if(!rep || rep->type!=REDIS_REPLY_ARRAY){
        return res;
    }
    for(size_t i = 0;i<rep->elements; ++i){
        res.emplace_back(rep->element[i]->str);
    }
    return res;
}

//=========================集合相关命令=========================
int CRedis::zsetAddItem(const string &zset, double score, const string &name) {
    std::vector<string> params = {zset, std::to_string(score), name};
    RedisReply rep = sendACommand("zadd", params);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return 0;
    }
    return rep->integer;
}

int CRedis::zsetAddItems(const string &zset,
        const std::vector<std::pair<double, string>>& items) {
    std::vector<string> params;
    for(const auto& p:items){
        params.push_back(std::to_string(p.first));
        params.push_back(p.second);
    }
    RedisReply rep = sendACommand("zadd", zset, params);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return 0;
    }
    return rep->integer;
}

double CRedis::zsetGetScore(const string &zset, const string &name) {
    RedisReply rep = sendACommand("zscore", zset, name);
    if(!rep || rep->type!=REDIS_REPLY_STRING || rep->str[0]==0){
        return DOUBLE_NUL;
    }

    return atof(rep->str);
}

double CRedis::zsetIncrScore(const string &zset, const string &name, double incr) {
    std::vector<string> params = {zset, std::to_string(incr), name};
    RedisReply rep = sendACommand("zincrby", params);
    if(!rep || rep->type!=REDIS_REPLY_STRING || rep->str[0]==0){
        return DOUBLE_NUL;
    }

    return atof(rep->str);
}

int CRedis::zsetLength(const string &zset) {
    string cmd = " " + zset;
    RedisReply rep = sendACommand("zcard", zset);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return 0;
    }
    return rep->integer;
}

std::vector<string> CRedis::zsetGetNameByScoreRange(
        const string &zset, double low, double high) {
    std::vector<string> res, params = {zset, std::to_string(low), std::to_string(high)};
    RedisReply rep = sendACommand("zcount", params);
    if(!rep || rep->type!=REDIS_REPLY_ARRAY){
        return res;
    }
    for(size_t i = 0;i<rep->elements; ++i){
        res.emplace_back(rep->element[i]->str);
    }
    return res;
}

std::vector<std::pair<string, double>> CRedis::zsetGetItemByScoreRange(
        const string &zset, double low, double high) {
    std::vector<string> params = {zset, std::to_string(low), std::to_string(high), "withscores"};
    std::vector<std::pair<string, double>> res;
    RedisReply rep = sendACommand("zcount", params);
    if(!rep || rep->type!=REDIS_REPLY_ARRAY){
        return res;
    }
    for(size_t i = 0;i<rep->elements; i+=2){
        res.emplace_back(std::make_pair(
                rep->element[i]->str, atof(rep->element[i+1]->str)));
    }
    return res;
}

std::vector<string> CRedis::zsetGetNameByIndex(
        const string &zset, int start, int end) {
    std::vector<string> res, params = {zset, std::to_string(start), std::to_string(end)};
    RedisReply rep = sendACommand("zrange", params);
    if(!rep || rep->type!=REDIS_REPLY_ARRAY){
        return res;
    }
    for(size_t i = 0;i<rep->elements; ++i){
        res.emplace_back(rep->element[i]->str);
    }
    return res;
}

std::vector<std::pair<string, double>> CRedis::zsetGetItemByIndex(
        const string &zset, int start, int end) {
    std::vector<string> params = {zset, std::to_string(start), std::to_string(end), "withscores"};
    std::vector<std::pair<string, double>> res;
    RedisReply rep = sendACommand("zrange", params);
    if(!rep || rep->type!=REDIS_REPLY_ARRAY){
        return res;
    }
    for(size_t i = 0;i<rep->elements; i+=2){
        res.emplace_back(std::make_pair(
                rep->element[i]->str, atof(rep->element[i+1]->str)));
    }
    return res;
}

std::vector<std::pair<string, double>> CRedis::zsetGetItemByIndexRev(
        const string &zset, int start, int end) {
    std::vector<string> params = {zset, std::to_string(start), std::to_string(end), "withscores"};
    std::vector<std::pair<string, double>> res;
    RedisReply rep = sendACommand("zrevrange", params);
    if(!rep || rep->type!=REDIS_REPLY_ARRAY){
        return res;
    }
    for(size_t i = 0;i<rep->elements; i+=2){
        res.emplace_back(std::make_pair(
                rep->element[i]->str, atof(rep->element[i+1]->str)));
    }
    return res;
}

int CRedis::zsetGetRank(const string &zset, const string &name) {
    RedisReply rep = sendACommand("zrank", zset, name);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return INT_NULL;
    }
    return rep->integer;
}

int CRedis::zsetGetRankRev(const string &zset, const string &name) {
    string cmd = " " + zset + " " + name;
    RedisReply rep = sendACommand("zrevrank", zset, name);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return INT_NULL;
    }
    return rep->integer;
}

int CRedis::zsetDelItemByName(const string &zset, const string &name) {
    string cmd = " " + zset + " " + name;
    RedisReply rep = sendACommand("zrem", zset, name);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return INT_NULL;
    }
    return rep->integer;
}

int CRedis::zsetDelItemsByName(const string &zset, const std::vector<string> &names) {
    RedisReply rep = sendACommand("zrem", zset, names);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return INT_NULL;
    }
    return rep->integer;
}

int CRedis::zsetDelItemsByRank(const string &zset, int start, int end) {
    std::vector<string> params = {zset, std::to_string(start), std::to_string(end)};
    RedisReply rep = sendACommand("zremrangebyrank", params);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return INT_NULL;
    }
    return rep->integer;
}

int CRedis::zsetDelItemsByScore(const string &zset, double low, double high) {
    std::vector<string> params = {zset, std::to_string(low), std::to_string(high)};
    RedisReply rep = sendACommand("zremrangebyscore", params);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return INT_NULL;
    }
    return rep->integer;
}

//=========================Bitset相关命令=========================
bool CRedis::bsSetBit(const string &bs, int index, bool bit) {
    std::vector<string> params = {bs, std::to_string(index), std::to_string(bit)};
    RedisReply rep = sendACommand("setbit", params);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return false;
    }
    return rep->integer == 1;
}

bool CRedis::bsGetBit(const string &bs, int index) {
    RedisReply rep = sendACommand("getbit", bs, std::to_string(index));
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return false;
    }
    return rep->integer == 1;
}

int CRedis::bsBitCount(const string &bs, int start, int end) {
    std::vector<string> params = {bs, std::to_string(start), std::to_string(end)};
    RedisReply rep = sendACommand("bitcount", params);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return false;
    }
    return rep->integer;
}

int CRedis::bsGetFirstIndex(const string &bs, bool bit, int start, int end) {
    std::vector<string> params = {bs, std::to_string(bit), std::to_string(start), std::to_string(end)};
    RedisReply rep = sendACommand("bitpos", params);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return false;
    }
    return rep->integer;
}