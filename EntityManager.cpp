//
// Created by gjk on 2020/2/10.
//
#include <unordered_set>
#include "EntityManager.h"
#include "Redis.h"
#include "Logger.h"
#include "FriendCroup.h"

User::User(const std::vector<std::pair<std::string, std::string>>& userinfo):
User(){
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

//==========================================================
Group::Group(uint32_t createUserId, const std::string& groupName):
m_groupId(0), m_ownerId(createUserId), m_groupName(groupName)
{
    m_groupMembers.insert(createUserId);
}

//==========================================================
bool EntityManager::init(const std::string &redishost,
        int redisport, const std::string &redispass) {
    //连接redis服务器
    m_redis = CRedis::connect(redishost, redisport, redispass);
    if(!m_redis){
        LOGE("redis connected failed");
        return false;
    }
    //初始化的时候就把所有的用户都加载出来
    if(!loadAllUsers()){
        return false;
    }
    if(!loadAllGroups()){
        return false;
    }
    return true;
}

bool EntityManager::loadAllUsers() {
    //先获取最大用户ID
    auto maxUserId = m_redis->strGet("user:maxid:str");
    m_maxUserId = atoi(maxUserId.c_str());
    if(m_maxUserId==0){
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
        m_users[nId] = user;
        m_account2Uid[user->m_userAccount] = nId;
    }
    LOGI("All users loaded, the number of users is %d", m_users.size());
    return true;
}

bool EntityManager::loadAllGroups() {
    auto maxGroupId = m_redis->strGet("group:maxid:str");
    m_maxGroupId = atoi(maxGroupId.c_str());

    //读取当前所有用户的uid
    std::vector<std::string> groupIds = m_redis->setGetKeys("group:gid:set");
    for(const string& strId: groupIds){
        //获取指定gid的群组信息
        auto groupInfo = m_redis->hashGetKeyValues("group:" + strId + ":hash");
        uint32_t gId = atoi(strId.c_str());
        GroupPtr group(new Group());
        group->m_groupId = gId;
        for(const auto& ginfo:groupInfo){
            if(ginfo.first=="name")group->m_groupName = ginfo.second;
            else if(ginfo.first=="ownerid")group->m_ownerId = atoi(ginfo.second.c_str());
        }

        //获取群组成员列表
        auto members = m_redis->setGetKeys("group:"+ strId + "members:set");
        for(const auto& member:members){    //添加好友列表信息
            uint32_t uid = atoi(member.c_str());
            group->m_groupMembers.insert(uid);
        }

        //将数据保存起来
        m_groups[gId] = group;
    }
    LOGI("All groups loaded, the number of groups is %d", m_groups.size());
    return true;
}

int EntityManager::addNewUser(const UserPtr& user) {
    if(user->m_userAccount.empty()){    //账号为空，不能进行注册
        return 0;
    }
    auto it = m_account2Uid.find(user->m_userAccount);
    if(it != m_account2Uid.end()){  //已存在该用户
        return 0;
    }

    //添加新用户的话，他还没有uid
    user->m_userId = ++m_maxUserId;
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
    //将最大id写入redis
    if(m_redis->strSet("user:maxid:str", std::to_string(m_maxUserId))){
        return 0;
    }
    //更新服务器内存中的数据
    m_users[user->m_userId] = user;
    m_account2Uid[user->m_userAccount] = user->m_userId;
    LOGI("Add new user success, his uid is %d", user->m_userId);
    return user->m_userId;
}

UserPtr EntityManager::getUserByUid(uint32_t uid) {
    auto it = m_users.find(uid);
    if(it==m_users.end())return nullptr;
    return it->second;
}

bool EntityManager::updateUserInfo(uint32_t uid) {
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

bool EntityManager::updateUserPassowrd(uint32_t uid) {
    auto user = getUserByUid(uid);
    if(!user)return false;

    return m_redis->hashSetKeyValue("user:"+std::to_string(uid)+":hash",
            "userpass", user->m_userPassword);
}

UserPtr EntityManager::getUserByAccount(const std::string &account) {
    auto it = m_account2Uid.find(account);
    if(it==m_account2Uid.end())return nullptr;
    auto user = m_users.find(it->second);
    if(user == m_users.end())return nullptr;
    return user->second;
}

std::vector<uint32_t> EntityManager::getFriendListById(uint32_t uid) {
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


int EntityManager::addNewGroup(const GroupPtr &group) {
    group->m_groupId = ++m_maxGroupId;
    //将群组信息写到redis中
    std::vector<std::pair<string, string>> infos;
    infos.emplace_back(std::make_pair("ownerid", std::to_string(group->m_groupId)));
    infos.emplace_back(std::make_pair("name", group->m_groupName));
    if(!m_redis->hashSetKeyValues(
            "group:" + std::to_string(group->m_groupId) + ":hash", infos)){
        return 0;
    }
    if(!m_redis->setAddKey("group:gid:set", std::to_string(group->m_groupId))){
        return 0;
    }
    if(!m_redis->strSet("group:maxid:str", std::to_string(m_maxGroupId))){
        return 0;
    }

    m_groups[group->m_groupId] = group;
    return group->m_groupId;
}

GroupPtr EntityManager::getGroupByGid(uint32_t gid) {
    auto it = m_groups.find(gid);
    if(it == m_groups.end()){
        return nullptr;
    }
    return it->second;
}

bool EntityManager::updateGroupInfo(uint32_t gid) {
    auto group = getGroupByGid(gid);
    if(!group){
        return false;
    }

    std::vector<std::pair<string, string>> infos;
    infos.emplace_back(std::make_pair("ownerid", std::to_string(group->m_groupId)));
    infos.emplace_back(std::make_pair("name", group->m_groupName));

    return m_redis->hashSetKeyValues(
            "group:" + std::to_string(group->m_groupId) + ":hash", infos);
}

std::set<uint32_t> EntityManager::getGroupMembers(uint32_t gid) {
    auto group = getGroupByGid(gid);
    if(!group){
        return std::set<uint32_t>();
    }
    return group->m_groupMembers;
}