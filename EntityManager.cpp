//
// Created by gjk on 2020/2/10.
//
#include <unordered_set>
#include "EntityManager.h"
#include "Redis.h"
#include "Logger.h"
#include "FriendCroup.h"

User::User():m_userId(0),m_onlineType(0),m_clientType(0),
m_gender(0), m_faceType(0), m_birthday(19920408)
{

}

User::User(const std::vector<std::pair<std::string, std::string>>& userinfo):
User(){
    for(const auto& info : userinfo){
        if(info.first=="uid"){
            m_userId = atoi(info.second.c_str());
        }else if(info.first=="gender"){
            m_gender = atoi(info.second.c_str());
        }else if(info.first=="facetype"){
            m_faceType = atoi(info.second.c_str());
        }else if(info.first=="birthday"){
            m_birthday = atoi(info.second.c_str());
        }else if(info.first=="nickname"){
            m_nickName = info.second;
        }else if(info.first=="userpass"){
            m_userPassword = info.second;
        }else if(info.first=="account"){
            m_userAccount = info.second;
        }else if(info.first=="phonenumber"){
            m_userPhoneNumber = info.second;
        }else if(info.first=="customface"){
            m_customFace = info.second;
        }else if(info.first=="signature"){
            m_signature = info.second;
        }else if(info.first=="address"){
            m_address = info.second;
        }else if(info.first=="mail"){
            m_mail = info.second;
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

    //获取最大好友分组id
    auto maxFriendGroupId = m_redis->strGet("friendgroup:maxid:str");
    m_maxFriendGroupId = atoi(maxFriendGroupId.c_str());

    //读取当前所有用户的uid
    std::vector<std::string> userids = m_redis->setGetKeys("user:uid:set");
    for(const string& strId: userids){
        //获取指定uid的用户信息
        auto userInfo = m_redis->hashGetKeyValues("user:" + strId + ":hash");
        uint32_t nId = atoi(strId.c_str());
        UserPtr user(new User(userInfo));

        //获取指定用户的好友分组的id
        auto fgids = m_redis->setGetKeys("user:"+ strId + ":friendgroup:set");
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

        //获取指定用户的所在群组id
        auto gids = m_redis->setGetKeys("user:"+ strId + ":groups:set");
        for(auto gid:gids){
            user->m_groups.insert(atoi(gid.c_str()));
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
    if(m_maxGroupId==0){
        m_maxGroupId = Group::GROUPID_BOUBDARY;
        m_redis->strSet("group:maxid:str", std::to_string(m_maxGroupId));
    }

    //读取当前所有群组的uid
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
        auto members = m_redis->setGetKeys("group:"+ strId + ":members:set");
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
    infos.emplace_back(std::make_pair("gender",std::to_string(user->m_gender)));
    infos.emplace_back(std::make_pair("facetype",std::to_string(user->m_faceType)));
    infos.emplace_back(std::make_pair("birthday",std::to_string(user->m_birthday)));
    infos.emplace_back(std::make_pair("nickname", user->m_nickName));
    infos.emplace_back(std::make_pair("userpass", user->m_userPassword));
    infos.emplace_back(std::make_pair("account", user->m_userAccount));
    infos.emplace_back(std::make_pair("phonenumber", user->m_userPhoneNumber));
    infos.emplace_back(std::make_pair("customface",user->m_customFace));
    infos.emplace_back(std::make_pair("signature",user->m_signature));
    infos.emplace_back(std::make_pair("address",user->m_address));
    infos.emplace_back(std::make_pair("mail",user->m_mail));

    //将用户信息写入redis
    if(!m_redis->hashSetKeyValues("user:"+std::to_string(user->m_userId)+":hash", infos)){
        return 0;
    }
    //将用户uid写入所有用户uid集合
    if(m_redis->setAddKey("user:uid:set", std::to_string(user->m_userId)) == 0){
        return 0;
    }
    //将最大id写入redis
    if(!m_redis->strSet("user:maxid:str", std::to_string(m_maxUserId))){
        return 0;
    }

    //更新服务器内存中的数据
    m_users[user->m_userId] = user;
    m_account2Uid[user->m_userAccount] = user->m_userId;

    //为用户添加默认好友分组
    addFriendGroup(user->m_userId, CFriendGroup::DEFAULT_NAME);
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
    infos.emplace_back(std::make_pair("gender",std::to_string(user->m_gender)));
    infos.emplace_back(std::make_pair("facetype",std::to_string(user->m_faceType)));
    infos.emplace_back(std::make_pair("birthday",std::to_string(user->m_birthday)));
    infos.emplace_back(std::make_pair("nickname", user->m_nickName));
//    infos.emplace_back(std::make_pair("userpass", user->m_userPassword));
//    infos.emplace_back(std::make_pair("account", user->m_userAccount));
    infos.emplace_back(std::make_pair("phonenumber", user->m_userPhoneNumber));
    infos.emplace_back(std::make_pair("customface",user->m_customFace));
    infos.emplace_back(std::make_pair("signature",user->m_signature));
    infos.emplace_back(std::make_pair("address",user->m_address));
    infos.emplace_back(std::make_pair("mail",user->m_mail));

    //将用户信息写入redis
    return m_redis->hashSetKeyValues("user:"+std::to_string(uid)+":hash", infos);
}

bool EntityManager::updateUserPassword(uint32_t uid) {
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

bool EntityManager::isFriend(uint32_t userA, uint32_t userB) {
    auto user = getUserByUid(userA);
    if(!user){
        return false;
    }
    for(const auto& fgroup:user->m_friendGroups){
        if(fgroup->existsUser(userB)){
            return true;
        }
    }
    return false;
}

bool EntityManager::makeFriendRelation(uint32_t userAid, uint32_t userBid) {
    auto fgroupOfA = getDefaultFriendGroup(userAid);
    auto fgroupOfB = getDefaultFriendGroup(userBid);
    if(!fgroupOfA || !fgroupOfB){
        return false;
    }

//    friendgroup:gid:userids:set
    if(!m_redis->setAddKey("friendgroup:"+
    std::to_string(fgroupOfA->getId())+":userids:set", std::to_string(userBid))){
        return false;
    }
    if(!m_redis->setAddKey("friendgroup:"+
    std::to_string(fgroupOfB->getId())+":userids:set", std::to_string(userAid))){
        return false;
    }

    fgroupOfA->addUser(userBid);
    fgroupOfB->addUser(userAid);
    LOGD("User %d and %d makeup friend relation", userAid, userBid);
    return true;
}

bool EntityManager::breakFriendRelation(uint32_t userAid, uint32_t userBid) {
    auto fgroupOfA = getDefaultFriendGroup(userAid);
    auto fgroupOfB = getDefaultFriendGroup(userBid);
    if(!fgroupOfA || !fgroupOfB){
        return false;
    }

//    friendgroup:gid:userids:set
    if(!m_redis->setDeleteKey("friendgroup:"+
                           std::to_string(fgroupOfA->getId())+":userids:set", std::to_string(userBid))){
        return false;
    }
    if(!m_redis->setDeleteKey("friendgroup:"+
                           std::to_string(fgroupOfB->getId())+":userids:set", std::to_string(userAid))){
        return false;
    }

    LOGD("User %d and %d breakup friend relation", userAid, userBid);
    fgroupOfA->delUser(userBid);
    fgroupOfB->delUser(userAid);
    return true;
}

bool EntityManager::addFriendGroup(uint32_t uid, const std::string &name) {
    auto user = getUserByUid(uid);
    if(!user){
        return false;
    }
    for(const auto &fg : user->m_friendGroups){
        if(fg->getName() == name){  //存在相同名字的
            return false;
        }
    }
    user->m_friendGroups.emplace_back(new CFriendGroup(++m_maxFriendGroupId, name));

    //将好友分组数据写到redis中
    auto strFGroupId = std::to_string(m_maxFriendGroupId);
    if(!m_redis->setAddKey("user:" + std::to_string(uid) + ":friendgroup:set", strFGroupId)){
        return false;
    }

    if(!m_redis->strSet("friendgroup:"+strFGroupId+":name:str", name)){
        return false;
    }

    //将最大id写入redis
    if(!m_redis->strSet("friendgroup:maxid:str", std::to_string(m_maxFriendGroupId))){
        return false;
    }
    LOGD("User %s(%d) add friend group '%s'(%d)", user->m_userAccount.c_str(),
            uid, name.c_str(), m_maxFriendGroupId);
    return true;
}

bool EntityManager::delFriendGroup(uint32_t uid, const std::string &name) {
    if(name==CFriendGroup::DEFAULT_NAME){   //默认分组不能删除
        return false;
    }
    auto user = getUserByUid(uid);
    if(!user){
        return false;
    }
    auto tar = user->m_friendGroups.end(), def = tar;
    for(auto it = user->m_friendGroups.begin(); it!=user->m_friendGroups.end(); ++it){
        if((*it)->getName() == name){  //找到那个分组
            tar = it;
        }
        if((*it)->getName() == CFriendGroup::DEFAULT_NAME){
            def = it;
        }
    }
    if(tar==user->m_friendGroups.end() ||
        def==user->m_friendGroups.end()){
        return false;
    }

    if(!(*tar)->getUserIds().empty()){
        //将待删除分组内的用户移动到默认分组
        if(!copyUsersToOtherFGroup(uid,(*tar)->getId(), (*def)->getId())){
            return false;
        }
    }

    //开始删除
    auto strId = std::to_string((*tar)->getId());
    if(!m_redis->deleteKey("friendgroup:"+strId+":name:str")){
        return false;
    }
    if(!m_redis->deleteKey("friendgroup:"+strId+":userids:set")){
        return false;
    }
    if(!m_redis->setDeleteKey("user:"+std::to_string(uid)+":friendgroup:set", strId)){
        return false;
    }

    LOGD("User %s delete friend gorup %s", user->m_userAccount.c_str(), (*tar)->getName().c_str());
    //从用户好友列表中删除
    user->m_friendGroups.erase(tar);
    return true;
}

std::shared_ptr<CFriendGroup> EntityManager::getDefaultFriendGroup(uint32_t uid) {
    auto user = getUserByUid(uid);
    if(!user){
        return nullptr;
    }
    for(const auto& fgroup:user->m_friendGroups){
        if(fgroup->getName()==CFriendGroup::DEFAULT_NAME){
            return fgroup;
        }
    }
    return nullptr;
}

bool EntityManager::modifyFriendGroup(uint32_t uid,
        const std::string &newName, const std::string &oldName) {
    if(oldName==CFriendGroup::DEFAULT_NAME ||
        newName==CFriendGroup::DEFAULT_NAME){   //默认分组不能修改
        return false;
    }
    auto user = getUserByUid(uid);
    if(!user){
        return false;
    }

    std::shared_ptr<CFriendGroup> tar;
    for(auto& fg:user->m_friendGroups){
        if(fg->getName()==oldName){
            tar = fg;
            break;
        }
    }
    if(!tar){   //旧名字不存在
        return false;
    }
    //修改
    if(!m_redis->strSet("friendgroup:"+std::to_string(tar->getId())+":name:str", newName)){
        return false;
    }
    LOGD("User %s change friend group name %s to %s",
            user->m_userAccount.c_str(), oldName.c_str(), newName.c_str());
    tar->setName(newName);
    return true;
}

bool EntityManager::moveUserToOtherFGroup(uint32_t uid, uint32_t fuid,
        uint32_t fromFGroupId, uint32_t toFGroupId) {
    if(fromFGroupId==toFGroupId){
        return true;
    }
    auto user = getUserByUid(uid);
    if(!user){
        return false;
    }

    std::shared_ptr<CFriendGroup> from,to;
    for(const auto& fg:user->m_friendGroups){
        if(fg->getId()==fromFGroupId)from = fg;
        if(fg->getId()==toFGroupId)to = fg;
    }

    if(!m_redis->setDeleteKey("friendgroup:" +std::to_string(fromFGroupId)+ ":userids:set",
                              std::to_string(fuid))){
        return false;
    }
    if(!m_redis->setAddKey("friendgroup:" +std::to_string(toFGroupId)+ ":userids:set",
                std::to_string(fuid))){
        return false;
    }
    LOGD("User:%d Move user:%d from %s to %s", uid, fuid,
            from->getName().c_str(), to->getName().c_str());
    from->delUser(fuid);
    to->addUser(fuid);
    return true;
}

bool EntityManager::copyUsersToOtherFGroup(uint32_t uid,
        uint32_t fromFGroupId, uint32_t toFGroupId) {
    if(fromFGroupId==toFGroupId){
        return true;
    }
    auto user = getUserByUid(uid);
    if(!user){
        return false;
    }
    std::shared_ptr<CFriendGroup> from,to;
    for(const auto& fg:user->m_friendGroups){
        if(fg->getId()==fromFGroupId)from = fg;
        if(fg->getId()==toFGroupId)to = fg;
    }
    //列表是空的不用复制好友，直接返回
    if(from->getUserIds().empty()){
        return true;
    }

    std::vector<std::string> uids;
    for(auto id:from->getUserIds()){
        uids.push_back(std::to_string(id));
    }

    if(!m_redis->setAddKeys("friendgroup:" +std::to_string(toFGroupId)+ ":userids:set", uids)){
        return false;
    }
    to->addUsers(from->getUserIds());
    from->delAllUser();
    return true;
}

int EntityManager::addNewGroup(const GroupPtr &group) {
    group->m_groupId = ++m_maxGroupId;

    //将群组信息写到redis中
    std::vector<std::pair<string, string>> infos;
    infos.emplace_back(std::make_pair("ownerid", std::to_string(group->m_ownerId)));
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
    joinGroup(group->m_groupId, group->m_ownerId);

    LOGD("Create group success name=%s, owner=%d, newGroupId=%d",
         group->m_groupName.c_str(), group->m_ownerId, group->m_groupId);
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

bool EntityManager::joinGroup(uint32_t gid, uint32_t userId) {
    auto group = getGroupByGid(gid);
    if(!group){
        return false;
    }
    auto user = getUserByUid(userId);
    if(!user){
        return false;
    }

    //添加userid到redis中
    if(!m_redis->setAddKey("group:"+std::to_string(gid)+":members:set",
            std::to_string(userId))){
        return false;
    }
    if(!m_redis->setAddKey("user:"+std::to_string(userId)+":groups:set",
            std::to_string(gid))){
        return false;
    }
    group->m_groupMembers.insert(userId);
    user->m_groups.insert(gid);
    LOGD("User %s joined group %s", user->m_userAccount.c_str(), group->m_groupName.c_str());
    return true;
}

bool EntityManager::quitGroup(uint32_t gid, uint32_t userId) {
    auto group = getGroupByGid(gid);
    if(!group){
        return false;
    }
    auto user = getUserByUid(userId);
    if(!user){
        return false;
    }

    //删除userid
    if(!m_redis->setDeleteKey("group:"+std::to_string(gid)+":members:set",
                           std::to_string(userId))){
        return false;
    }
    if(!m_redis->setDeleteKey("user:"+std::to_string(userId)+":groups:set",
                           std::to_string(gid))){
        return false;
    }

    LOGD("User %s quit group %s", user->m_userAccount.c_str(), group->m_groupName.c_str());
    group->m_groupMembers.erase(userId);
    user->m_groups.erase(gid);
    return true;
}

std::set<uint32_t> EntityManager::getGroupMembers(uint32_t gid) {
    auto group = getGroupByGid(gid);
    if(!group){
        return std::set<uint32_t>();
    }
    return group->m_groupMembers;
}

bool EntityManager::isGroupMember(uint32_t groupId, uint32_t userId) {
    auto group = getGroupByGid(groupId);
    if(!group){
        return false;
    }
    return group->m_groupMembers.find(userId) != group->m_groupMembers.end();
}

void EntityManager::addOfflineMsg(uint32_t uid, const std::string &msg) {
    m_offlineMsgs[uid].push_back(msg);
}

std::vector<std::string> EntityManager::getOfflineMsgs(uint32_t uid) {
    std::vector<std::string> msgs;
    auto it = m_offlineMsgs.find(uid);
    if(it == m_offlineMsgs.end()){
        return msgs;
    }
    msgs = it->second;
    m_offlineMsgs.erase(it);
    return msgs;
}
