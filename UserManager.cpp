//
// Created by gjk on 2020/2/10.
//
#include <unordered_set>
#include "UserManager.h"
#include "Redis.h"
#include "Logger.h"
#include "FriendCroup.h"

User::User(const std::vector<std::pair<std::string, std::string>>& userinfo):User(){
    for(const auto& info : userinfo){
        if(info.first=="uid"){
            m_userId = atoi(info.second.c_str());
        }else if(info.first=="nickname"){
            m_nickName = info.second;
        }else if(info.first=="userpass"){
            m_userPassword = info.second;
        }else if(info.first=="account"){
            m_userAccount = info.second;
            m_userPhoneNum = info.second;
        }
    }
}

bool UserManager::init(const std::string &redishost,
        int redisport, const std::string &redispass) {
    //连接redis服务器
    m_redis = CRedis::connect(redishost, redisport, redispass);
    if(!m_redis){
        LOGE("redis connected failed");
        return false;
    }
    //初始化的时候就把所有的用户都加载出来
    return loadAllUsers();
}

bool UserManager::loadAllUsers() {
    //先取出所有用户的uid集合，在通过uid集合一个个的取出用户信息
    m_allUserNumber = m_redis->setLength("user:uid:set");
    if(m_allUserNumber==0){
        return true;
    }
    //读取当前所有用户的uid
    std::vector<std::string> userids = m_redis->setGetKeys("user:uid:set");
    for(const string& strId: userids){
        //获取指定uid的用户信息
        auto userInfo = m_redis->hashGetKeyValues("user:" + strId + ":hash");
        uint32_t nId = atoi(strId.c_str());
        UserPtr user(new User(userInfo));

        //获取指定用户的好友分组的id
        auto fgids = m_redis->setGetKeys("user:"+ strId + "friendgroup:set");
        for(const auto& fgid:fgids){    //添加好友列表信息
            auto fgname = m_redis->strGet("friendgroup:"+ fgid +":name:str");
            uint32_t nFgid = atoi(fgid.c_str());
            auto fguids = m_redis->setGetKeys("friendgroup:" + fgid +":userids:set");
            std::unordered_set<int> setUid;
            for(const auto& sUid:fguids){
                setUid.insert(atoi(sUid.c_str()));
            }
            user->m_friendGroups.emplace_back(new CFriendGroup(nFgid, fgname, setUid));
        }

        //将数据保存起来
        m_setAllUserId.insert(nId);
        m_users[nId] = user;
        m_account2Uid[user->m_userAccount] = nId;
    }
    LOGI("All users loaded, the number of users is %d", m_allUserNumber);
    return true;
}

int UserManager::addNewUser(const UserPtr& user) {
    if(user->m_userAccount.empty()){    //账号为空，不能进行注册
        return 0;
    }
    auto it = m_account2Uid.find(user->m_userAccount);
    if(it != m_account2Uid.end()){  //已存在该用户
        return 0;
    }

    //添加新用户的话，他还没有uid
    user->m_userId = ++m_allUserNumber;
    //先构造要写入redis的数据
    std::vector<std::pair<string, string>> infos;
    infos.emplace_back(std::make_pair("uid", std::to_string(user->m_userId)));
    infos.emplace_back(std::make_pair("nickname", user->m_nickName));
    infos.emplace_back(std::make_pair("userpass", user->m_userPassword));
    infos.emplace_back(std::make_pair("account", user->m_userAccount));
    infos.emplace_back(std::make_pair("phonenumber", user->m_userPhoneNum));
    //将用户信息写入redis
    if(!m_redis->hashSetKeyValues("user:"+std::to_string(user->m_userId)+":hash", infos)){
        return 0;
    }
    //将用户uid写入所有用户uid集合
    if(m_redis->setAddKey("user:uid:set", std::to_string(user->m_userId)) == 0){
        return 0;
    }

    //更新服务器内存中的数据
    m_users[user->m_userId] = user;
    m_setAllUserId.insert(user->m_userId);
    m_account2Uid[user->m_userAccount] = user->m_userId;
    LOGI("Add new user success, his uid is %d", user->m_userId);
    return user->m_userId;
}

UserPtr UserManager::getUserByUid(uint32_t uid) {
    auto it = m_users.find(uid);
    if(it==m_users.end())return nullptr;
    return it->second;
}

bool UserManager::updateUserInfo(uint32_t uid) {
    //先构造要写入redis的数据
    auto user = getUserByUid(uid);
    if(!user)return false;
    std::vector<std::pair<string, string>> infos;
    infos.emplace_back(std::make_pair("uid", std::to_string(user->m_userId)));
    infos.emplace_back(std::make_pair("nickname", user->m_nickName));
//    infos.emplace_back(std::make_pair("userpass", user->m_userPassword));
    infos.emplace_back(std::make_pair("account", user->m_userAccount));
    infos.emplace_back(std::make_pair("phonenumber", user->m_userPhoneNum));

    //将用户信息写入redis
    return m_redis->hashSetKeyValues("user:"+std::to_string(uid)+":hash", infos);
}

bool UserManager::updateUserPassowrd(uint32_t uid) {
    auto user = getUserByUid(uid);
    if(!user)return false;

    return m_redis->hashSetKeyValue("user:"+std::to_string(uid)+":hash",
            "userpass", user->m_userPassword);
}

UserPtr UserManager::getUserByAccount(const std::string &account) {
    auto it = m_account2Uid.find(account);
    if(it==m_account2Uid.end())return nullptr;
    auto user = m_users.find(it->second);
    if(user == m_users.end())return nullptr;
    return user->second;
}

std::vector<uint32_t> UserManager::getFriendListById(uint32_t uid) {
    std::vector<uint32_t> ret;
    auto user = getUserByUid(uid);
    if(!user){
        return ret;
    }
    for(const auto& fgroup:user->m_friendGroups){
        for(auto id:fgroup->getUserIds()){
            ret.push_back(id);
        }
    }
    return ret;
}