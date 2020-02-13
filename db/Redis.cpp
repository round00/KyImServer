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
        auto reply = sendACommand("auth " + m_password);
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

RedisReply CRedis::sendACommand(const string &cmd) {
    RedisReply reply(static_cast<redisReply*>(
            ::redisCommand(m_conn.get(), cmd.c_str())), freeReplyObject);
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

bool CRedis::isConnected() {
    RedisReply reply = sendACommand("ping");
    return reply && reply->type==REDIS_REPLY_STATUS
        && strcmp(reply->str, "PONG")==0;
}

bool CRedis::changeDb(int db) {
    RedisReply rep = sendACommand("select " + std::to_string(db));
    return rep && rep->type==REDIS_REPLY_STATUS
        &&strcmp(rep->str, "OK")==0;
}

bool CRedis::deleteKey(const string &key) {
    string cmd = "del " + key;
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return false;
    }
    return rep->integer==1;
}

//=========================字符串相关命令=========================
bool CRedis::strSet(const string &key, const string &value) {
    string cmd = "set " + key + " " + value;
    RedisReply rep = sendACommand(cmd);
    return rep && rep->type==REDIS_REPLY_STATUS
           &&strcmp(rep->str, "OK")==0;
}

string CRedis::strGet(const string &key) {
    string cmd = "get " + key;
    RedisReply reply = sendACommand(cmd);
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
    string cmd = "incr " + key;
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return -1;
    }
    return rep->integer;
}

LL CRedis::strIncrBy(const string &key, LL by) {
    string cmd = "incr " + key + " " +std::to_string(by);
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return -1;
    }
    return rep->integer;
}

LL CRedis::strDecr(const string &key) {
    string cmd = "decr " + key;
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return -1;
    }
    return rep->integer;
}

LL CRedis::strDecrBy(const string &key, LL by) {
    string cmd = "decr " + key + " "+std::to_string(by);
    RedisReply rep = sendACommand(cmd);
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
    string cmd = "lpsuh "+ key + " " + val;
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return 0;
    }
    return rep->integer;
}

int CRedis::listLpush(const string &key, const std::vector<string> &val) {
    if(val.empty())return 0;
    string cmd = "lpush " + key;
    for(const string& s:val){
        cmd += " " + s;
    }
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return 0;
    }
    return rep->integer;
}

int CRedis::listRpush(const string &key, int val) {
    return listRpush(key, std::to_string(val));
}

int CRedis::listRpush(const string &key, const string &val) {
    string cmd = "rpsuh "+ key + " " + val;
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return 0;
    }
    return rep->integer;
}

int CRedis::listRpush(const string &key, const std::vector<string> &val) {
    if(val.empty())return 0;
    string cmd = "rpush " + key;
    for(const string& s:val){
        cmd += " " + s;
    }
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return 0;
    }
    return rep->integer;
}

string CRedis::listLpop(const string &key) {
    string cmd = "lpop " + key;
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_STRING){
        return "";
    }
    return rep->str;
}

string CRedis::listRpop(const string &key) {
    string cmd = "rpop " + key;
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_STRING){
        return "";
    }
    return rep->str;
}

int CRedis::listDelete(const string &key, const string &val) {
    string cmd = "lrem " + key + " 0 " + val;
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return 0;
    }
    return rep->integer;
}

int CRedis::listLength(const string &key) {
    string cmd = "llen " + key;
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return 0;
    }
    return rep->integer;
}


string CRedis::listGetByIndex(const string &key, int index) {
    string cmd = "lindex " + key + " " + std::to_string(index);
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_STRING){
        return "";
    }
    return rep->str;
}

std::vector<string> CRedis::listGetByRange(const string &key, int start, int stop) {
    string cmd = "lrange " + key + " " +
            std::to_string(start) + " " + std::to_string(stop);
    std::vector<string> res;
    RedisReply rep = sendACommand(cmd);
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
    string cmd = "hset " + hash + " " + key + " " + val;
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return false;
    }
    return rep->integer>=0;
}

bool CRedis::hashSetKeyValues(const string &hash,
        const std::vector<std::pair<string, string>>& keyvals) {
    string cmd = "hmset " + hash;
    for(const auto& p:keyvals){
        cmd += " " + p.first + " " + p.second;
    }
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_STATUS){
        return false;
    }
    return strcmp(rep->str, "OK")==0;

}

string CRedis::hashGetValue(const string &hash, const string &key) {
    string cmd = "hget " + hash + " " + key;
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_STRING){
        return "";
    }
    return rep->str;
}

bool CRedis::hashExistKey(const string &hash, const string &key) {
    string cmd = "hexists " + hash + " " +key;
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return false;
    }
    return rep->integer==1;
}

int CRedis::hashDeleteKey(const string &hash, const string &key) {
    string cmd = "hdel " + hash + " " +key;
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return 0;
    }
    return rep->integer;
}

int CRedis::hashDeleteKeys(const string &hash, const std::vector<string> &keys) {
    string cmd = "hdel " + hash;
    for(const string& key:keys){
        cmd += " " + key;
    }
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return 0;
    }
    return rep->integer;
}

int CRedis::hashLength(const string &hash) {
    string cmd = "hlen " + hash;
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return 0;
    }
    return rep->integer;
}

int CRedis::hashAddByInt(const string &hash, const string &key, int by) {
    string cmd = "hincrby " + hash + " " + key + " " + std::to_string(by);
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return INT_NULL;
    }
    return rep->integer;
}

std::vector<string> CRedis::hashGetKeys(const string &hash) {
    string cmd = "hkeys " + hash;
    std::vector<string> res;
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_ARRAY){
        return res;
    }

    for(size_t i = 0; i<rep->elements; ++i){
        res.emplace_back(rep->element[i]->str);
    }
    return res;
}
std::vector<string> CRedis::hashGetValues(const string &hash) {
    string cmd = "hvals " + hash;
    std::vector<string> res;
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_ARRAY){
        return res;
    }

    for(size_t i = 0; i<rep->elements; ++i){
        res.emplace_back(rep->element[i]->str);
    }
    return res;
}

std::vector<std::pair<string,string>> CRedis::hashGetKeyValues(const string &hash) {
    string cmd = "hgetall " + hash;
    std::vector<std::pair<string,string>> res;
    RedisReply rep = sendACommand(cmd);
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
    string cmd = "sadd " + set + " " + key;
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return 0;
    }
    return rep->integer;
}

int CRedis::setAddKeys(const string &set, const std::vector<string> &keys) {
    string cmd = "sadd " + set;
    for(const string& key:keys){
        cmd += " " + key;
    }
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return 0;
    }
    return rep->integer;
}

bool CRedis::setExists(const string &set, const string &key) {
    string cmd = "sismember " + set + " " + key;
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return false;
    }
    return rep->integer==1;
}

string CRedis::setRandomDelete(const string &set) {
    string cmd = "spop " + set;
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_STRING){
        return "";
    }
    return rep->str;
}

string CRedis::setRandomGet(const string &set) {
    string cmd = "srandmember " + set;
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_STRING){
        return "";
    }
    return rep->str;
}

int CRedis::setDeleteKey(const string &set, const string &key) {
    string cmd = "srem " + set + " " + key;
    RedisReply rep = sendACommand(cmd);
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
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return false;
    }
    return rep->integer;
}

bool CRedis::setMoveKey(const string &from, const string &to, const string &key) {
    string cmd = "smove " + from + " " + to + " " + key;
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return false;
    }
    return rep->integer==1;
}

int CRedis::setLength(const string &set) {
    string cmd = "scard " + set;
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return 0;
    }
    return rep->integer;
}

std::vector<string> CRedis::setGetKeys(const string &set) {
    string cmd = "smembers " + set;
    std::vector<string> res;
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_ARRAY){
        return res;
    }
    for(size_t i = 0;i<rep->elements; ++i){
        res.emplace_back(rep->element[i]->str);
    }
    return res;
}

std::vector<string> CRedis::setIntersection(const std::vector<string>& sets) {
    string cmd = "sinter ";
    for(const string& s:sets){
        cmd += " " + s;
    }
    std::vector<string> res;
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_ARRAY){
        return res;
    }
    for(size_t i = 0;i<rep->elements; ++i){
        res.emplace_back(rep->element[i]->str);
    }
    return res;
}

std::vector<string> CRedis::setUnion(const std::vector<string>& sets) {
    string cmd = "sunion ";
    for(const string& s:sets){
        cmd += " " + s;
    }
    std::vector<string> res;
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_ARRAY){
        return res;
    }
    for(size_t i = 0;i<rep->elements; ++i){
        res.emplace_back(rep->element[i]->str);
    }
    return res;
}

std::vector<string> CRedis::setDifference(const std::vector<string>& sets) {
    string cmd = "sdiff ";
    for(const string& s:sets){
        cmd += " " + s;
    }
    std::vector<string> res;
    RedisReply rep = sendACommand(cmd);
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
    string cmd = "zadd " + zset + " " + std::to_string(score) + " " + name;
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return 0;
    }
    return rep->integer;
}

int CRedis::zsetAddItems(const string &zset,
        const std::vector<std::pair<double, string>>& items) {
    string cmd = "zadd " + zset;
    for(const auto& p:items){
        cmd += " " + std::to_string(p.first) + " " + p.second;
    }
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return 0;
    }
    return rep->integer;
}

double CRedis::zsetGetScore(const string &zset, const string &name) {
    string cmd = "zscore " + zset + " " + name;
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_STRING || rep->str[0]==0){
        return DOUBLE_NUL;
    }

    return atof(rep->str);
}

double CRedis::zsetIncrScore(const string &zset, const string &name, double incr) {
    string cmd = "zincrby " + zset + " " + std::to_string(incr) + " " + name;
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_STRING || rep->str[0]==0){
        return DOUBLE_NUL;
    }

    return atof(rep->str);
}

int CRedis::zsetLength(const string &zset) {
    string cmd = "zcard " + zset;
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return 0;
    }
    return rep->integer;
}

std::vector<string> CRedis::zsetGetNameByScoreRange(
        const string &zset, double low, double high) {
    string cmd = "zcount " + zset + " " + std::to_string(low) +
            " " + std::to_string(high);
    std::vector<string> res;
    RedisReply rep = sendACommand(cmd);
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
    string cmd = "zcount " + zset + " " + std::to_string(low) +
            " " + std::to_string(high) + " withscores";
    std::vector<std::pair<string, double>> res;
    RedisReply rep = sendACommand(cmd);
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
    string cmd = "zrange " + zset + " " + std::to_string(start) +
                 " " + std::to_string(end);
    std::vector<string> res;
    RedisReply rep = sendACommand(cmd);
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
    string cmd = "zrange " + zset + " " + std::to_string(start) +
                 " " + std::to_string(end) + " withscores";
    std::vector<std::pair<string, double>> res;
    RedisReply rep = sendACommand(cmd);
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
    string cmd = "zrevrange " + zset + " " + std::to_string(start) +
                 " " + std::to_string(end) + " withscores";
    std::vector<std::pair<string, double>> res;
    RedisReply rep = sendACommand(cmd);
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
    string cmd = "zrank " + zset + " " + name;
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return INT_NULL;
    }
    return rep->integer;
}

int CRedis::zsetGetRankRev(const string &zset, const string &name) {
    string cmd = "zrevrank " + zset + " " + name;
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return INT_NULL;
    }
    return rep->integer;
}

int CRedis::zsetDelItemByName(const string &zset, const string &name) {
    string cmd = "zrem " + zset + " " + name;
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return INT_NULL;
    }
    return rep->integer;
}

int CRedis::zsetDelItemsByName(const string &zset, const std::vector<string> &names) {
    string cmd = "zrem " + zset;
    for(const string& name:names){
        cmd += " " + name;
    }
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return INT_NULL;
    }
    return rep->integer;
}

int CRedis::zsetDelItemsByRank(const string &zset, int start, int end) {
    string cmd = "zremrangebyrank " + zset + " " +
            std::to_string(start) + " " + std::to_string(end);

    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return INT_NULL;
    }
    return rep->integer;
}

int CRedis::zsetDelItemsByScore(const string &zset, double low, double high) {
    string cmd = "zremrangebyscore " + zset + " " +
                 std::to_string(low) + " " + std::to_string(high);

    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return INT_NULL;
    }
    return rep->integer;
}

//=========================Bitset相关命令=========================
bool CRedis::bsSetBit(const string &bs, int index, bool bit) {
    string cmd = "setbit " + bs + " " +
            std::to_string(index) + " " +std::to_string(bit);
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return false;
    }
    return rep->integer == 1;
}

bool CRedis::bsGetBit(const string &bs, int index) {
    string cmd = "getbit " + bs + " " + std::to_string(index);
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return false;
    }
    return rep->integer == 1;
}

int CRedis::bsBitCount(const string &bs, int start, int end) {
    string cmd = "bitcount " + bs + " " +
            std::to_string(start) + " " + std::to_string(end);
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return false;
    }
    return rep->integer;
}

int CRedis::bsGetFirstIndex(const string &bs, bool bit, int start, int end) {
    string cmd = "bitpos " + bs + " " + std::to_string(bit) + " " +
                 std::to_string(start) + " " + std::to_string(end);
    RedisReply rep = sendACommand(cmd);
    if(!rep || rep->type!=REDIS_REPLY_INTEGER){
        return false;
    }
    return rep->integer;
}