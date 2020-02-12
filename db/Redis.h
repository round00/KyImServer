//
// Created by gjk on 2020/1/30.
//

#ifndef KYIMSERVER_REDIS_H
#define KYIMSERVER_REDIS_H

#include <string>
#include <memory>
#include <vector>
using std::string;

class CRedis;
struct redisContext;
struct redisReply;

typedef std::shared_ptr<CRedis>         RedisPtr;
typedef std::shared_ptr<redisReply>     RedisReply;
typedef std::shared_ptr<redisContext>   RedisConn;
typedef long long                       LL;

//数值型的非法值
const int INT_NULL = 0x80000000;
const double DOUBLE_NUL = -9999999999999999.0;
class CRedis {
public:
    static RedisPtr connect(const string& ip, int port, const string& passwd="");
    //测试是否已经连接
    bool        isConnected();
    //切换db，redis有16个db，初始为0号db
    bool        changeDb(int db);
    //删除键
    bool        deleteKey(const string& key);

    //http://redisdoc.com/
    //=========================字符串相关命令=========================
    bool        strSet(const string& key, const string& value);
    string      strGet(const string& key);
    LL          strGetNumValue(const string& key);
    //以下几个增加/减少到命令，成功时返回执行后的结果，失败返回INT_NULL
    LL          strIncr(const string& key);
    LL          strIncrBy(const string& key, LL by);
    LL          strDecr(const string& key);
    LL          strDecrBy(const string& key, LL by);

    //=========================列表相关命令=========================
    int         listLpush(const string& key, int val);
    int         listLpush(const string& key, const string& val);
    int         listLpush(const string& key, const std::vector<string>& val);
    int         listRpush(const string& key, int val);
    int         listRpush(const string& key, const string& val);
    int         listRpush(const string& key, const std::vector<string>& val);
    //左/右弹出命令，成功则返回弹出的头节点，失败返回空
    string      listLpop(const string& key);
    string      listRpop(const string& key);
    //删除key中所有的val
    int         listDelete(const string& key, const string& val);
    int         listLength(const string& key);
    //返回key中下标为index的元素，可以类似python那种负数下标。不存在的话返回空
    string      listGetByIndex(const string& key, int index);
    //返回[start, stop]之间的元素，注意这里是闭区间。
    //获取列表所有元素可以取区间=[0, -1]
    std::vector<string> listGetByRange(const string& key, int start, int stop);

    //=========================哈希表相关命令=========================
    //哈希表命令中第一个参数为hash表的键
    //增加/修改key对应的value值
    bool        hashSetKeyValue(const string& hash, const string& key, const string& val);
    bool        hashSetKeyValues(const string& hash,
            const std::vector<std::pair<string,string>>& keyvals);
    string      hashGetValue(const string& hash, const string& key);
    bool        hashExistKey(const string& hash, const string& key);
    //删除key的返回值是成功删除key的数目
    int         hashDeleteKey(const string& hash, const string& key);
    int         hashDeleteKeys(const string& hash, const std::vector<string>& keys);
    //获取hash表中key的数目
    int         hashLength(const string& hash);
    //给指定key的value增加by，成功返回key增加后的值，失败返回-1，如果和需求有冲突的话再修改
    int         hashAddByInt(const string& hash, const string& key, int by);
    //获取所有的*
    std::vector<string> hashGetKeys(const string& hash);
    std::vector<string> hashGetValues(const string& hash);
    std::vector<std::pair<string,string>> hashGetKeyValues(const string& hash);

    //=========================集合相关命令=========================
    //集合命令中第一个参数为集合的键
    //添加key到set集合中
    int         setAddKey(const string& set, const string& key);
    int         setAddKeys(const string& set, const std::vector<string>& keys);
    bool        setExists(const string& set, const string& key);
    //随机删除集合中的一个元素，返回删除的值
    string      setRandomDelete(const string& set);
    //随机获取集合中的一个元素
    string      setRandomGet(const string& set);
    int         setDeleteKey(const string& set, const string& key);
    int         setDeleteKeys(const string& set, const std::vector<string>& keys);
    //将集合from中的key移动到集合to中
    bool        setMoveKey(const string& from, const string& to, const string& key);
    int         setLength(const string& set);
    std::vector<string> setGetKeys(const string& set);
    //下面是集合的交并差运算
    std::vector<string> setIntersection(const std::vector<string>& sets);
    std::vector<string> setUnion(const std::vector<string>& sets);
    std::vector<string> setDifference(const std::vector<string>& sets);

    //=========================有序集合相关命令=========================
    //有序集合命令中第一个参数为有序集合的键
    //添加一个item到有序集合中，包括它的名字和分数
    int         zsetAddItem(const string& zset, double score, const string& name);
    int         zsetAddItems(const string& zset,
            const std::vector<std::pair<double, string>>& items);
    //获取指定元素的分值
    double      zsetGetScore(const string& zset, const string& name);
    double      zsetIncrScore(const string& zset, const string& name, double incr);
    int         zsetLength(const string& zset);

    //获取指定分数范围的元素名字
    std::vector<string> zsetGetNameByScoreRange(const string& zset, double low, double high);
    //获取指定分数范围内的元素的值-分数对
    std::vector<std::pair<string, double>> zsetGetItemByScoreRange(
            const string& zset, double low, double high);
    //获取指定下标范围的元素名字
    std::vector<string> zsetGetNameByIndex(const string& zset, int start, int end);
    //获取指定下标范围内的元素的值-分数对
    //获取全部元素可以指定[0, -1]
    std::vector<std::pair<string, double>> zsetGetItemByIndex(
            const string& zset, int start, int end);
    //获取指定下标范围内的元素的值-分数对，从大->小
    std::vector<std::pair<string, double>> zsetGetItemByIndexRev(
            const string& zset, int start, int end);

    //返回指定name的排名，从小->大
    int         zsetGetRank(const string& zset, const string& name);
    //返回指定name的逆序排名，从大->小
    int         zsetGetRankRev(const string& zset, const string& name);

    //通过名字删除item
    int         zsetDelItemByName(const string& zset, const string& name);
    int         zsetDelItemsByName(
            const string& zset, const std::vector<string>& names);
    //通过排名删除item
    int         zsetDelItemsByRank(const string& zset, int start, int end);
    //通过分数范围删除item
    int         zsetDelItemsByScore(const string& zset, double low, double high);

    //=========================Bitset相关命令=========================
    bool        bsSetBit(const string& bs, int index, bool bit);
    bool        bsGetBit(const string& bs, int index);
    //获取指定key中的值1的个数
    int         bsBitCount(const string& bs, int start = 0, int end = -1);
    //获取bit在[start, end]内第一次出现的位置
    int         bsGetFirstIndex(const string& bs, bool bit = true,
            int start = 0, int end = -1);



private:
    CRedis(const string& ip, int port, const string& passwd)
        :m_hostIP(ip), m_port(port), m_password(passwd),
        m_conn(nullptr){}

    bool        init();
    RedisReply  sendACommand(const string& cmd);

private:
    string      m_hostIP;
    int         m_port;
    string      m_password;
    RedisConn   m_conn;
};


#endif //KYIMSERVER_REDIS_H
