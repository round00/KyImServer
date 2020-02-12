//
// Created by gjk on 2020/2/10.
//

#include "UserManager.h"
#include "Redis.h"
#include "Logger.h"


User::User(const std::vector<std::pair<std::string, std::string>>& userinfo):User(){
    for(const auto& info : userinfo){
        if(info.first=="uid"){
            m_userId = atoi(info.second.c_str());
        }else if(info.first=="username"){
            m_userName = info.second;
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
        //return true;
    }

    std::vector<std::string> userids = m_redis->setGetKeys("user:uid:set");
    for(const string& strId: userids){
        auto userInfo = m_redis->hashGetKeyValues("user:" + strId + ":hash");
        int nId = atoi(strId.c_str());
        UserPtr user(new User(userInfo));

        m_setAllUserId.insert(nId);
        m_users[nId] = user;
        m_account2Uid[user->m_userAccount] = nId;
    }
    LOGI("All users loaded, the number of users is %d", m_allUserNumber);
    return true;
}

int UserManager::addNewUser(const UserPtr& user) {
    //添加新用户的话，他还没有uid
    user->m_userId = ++m_allUserNumber;
    //先构造要写入redis的数据
    std::vector<std::pair<string, string>> infos;
    infos.emplace_back(std::make_pair("uid", std::to_string(user->m_userId)));
    infos.emplace_back(std::make_pair("username", user->m_userName));
    infos.emplace_back(std::make_pair("userpass", user->m_userPassword));
    infos.emplace_back(std::make_pair("account", user->m_userAccount));
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

UserPtr UserManager::getUserByUid(int uid) {
    auto it = m_users.find(uid);
    if(it==m_users.end())return nullptr;
    return it->second;
}

UserPtr UserManager::getUserByAccount(const std::string &account) {
    auto it = m_account2Uid.find(account);
    if(it==m_account2Uid.end())return nullptr;
    auto user = m_users.find(it->second);
    if(user == m_users.end())return nullptr;
    return user->second;
}