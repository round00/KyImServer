//
// Created by gjk on 2020/2/10.
//

#ifndef KYIMSERVER_ENTITYMANAGER_H
#define KYIMSERVER_ENTITYMANAGER_H

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <set>
#include <memory>
#include <vector>
#include "Redis.h"
#include "noncopyable.h"

class CFriendGroup;

struct User{
    User():m_userId(0),m_onlineType(0),m_clientType(0){}
    explicit User(const std::vector<std::pair<std::string, std::string>>& userinfo);


    uint32_t        m_userId;
    std::string     m_userAccount;
    std::string     m_nickName;
    std::string     m_userPassword;
    std::string     m_userPhoneNum;
    uint8_t         m_onlineType;       //0：离线，1：在线
    uint8_t         m_clientType;       //1：windows
    //用户好友列表
    std::vector<std::shared_ptr<CFriendGroup>>
                    m_friendGroups;
    //用户所在群组集合
    std::unordered_set<uint32_t>
                    m_groups;
};
typedef std::shared_ptr<User> UserPtr;

struct Group{
    Group():m_groupId(0), m_ownerId(0){}
    Group(uint32_t createUserId, const std::string& groupName);
    //群组id从这里开始，这是Flamgingo的规则，暂时先不改它
    static const uint32_t GROUPID_BOUBDARY = 0x0FFFFFFF;

    uint32_t        m_groupId;
    uint32_t        m_ownerId;
    std::string     m_groupName;
    std::set<uint32_t > m_groupMembers;
};
typedef std::shared_ptr<Group> GroupPtr;

//===============================================================
//用户、群组都看做是实体对象
class EntityManager : public noncopyable{
public:
    typedef std::unordered_map<uint32_t , UserPtr> UserMap;
    typedef std::unordered_map<std::string, uint32_t > AccountUidMap;
    typedef std::unordered_map<uint32_t , GroupPtr> GroupMap;

    static EntityManager& getInstance(){
        static EntityManager instance;
        return instance;
    }
    //初始化UserManage，应该在第一次使用UserManage实例之前就调用这个
    bool        init(const std::string& redishost, int redisport,
            const std::string& redispass);

    //=====================用户=====================
    //添加一个新用户，返回这个用户的uid，0无效
    int         addNewUser(const UserPtr& user);
    UserPtr     getUserByUid(uint32_t uid);
    //这里的更新指的是将数据更新到数据库中
    bool        updateUserInfo(uint32_t uid);
    bool        updateUserPassowrd(uint32_t uid);
    UserPtr     getUserByAccount(const std::string& account);
    //获取好友列表
    std::vector<uint32_t> getFriendListById(uint32_t uid);
    bool        isFriend(uint32_t userA, uint32_t userB);
    bool        makeFriendRelation(uint32_t userAid, uint32_t userBid);
    bool        breakFriendRelation(uint32_t userAid, uint32_t userBid);

    //=====================好友分组=====================
    bool        addFriendGroup(uint32_t uid, const std::string& name);
    bool        delFriendGroup(uint32_t uid, const std::string& name);
    std::shared_ptr<CFriendGroup> getDefaultFriendGroup(uint32_t uid);
    bool        modifyFriendGroup(uint32_t uid,
            const std::string& newName, const std::string& oldName);
    bool        moveUserToOtherFGroup(uint32_t uid, uint32_t fuid,
            uint32_t fromFGroupId, uint32_t toFGroupId);
    bool        copyUsersToOtherFGroup(uint32_t uid, uint32_t fromFGroupId, uint32_t toFGroupId);

    //=====================群组=====================
    int         addNewGroup(const GroupPtr& group);
    GroupPtr    getGroupByGid(uint32_t gid);
    bool        updateGroupInfo(uint32_t gid);
    bool        joinGroup(uint32_t gid, uint32_t userId);
    bool        quitGroup(uint32_t gid, uint32_t userId);
    //获取成员列表
    std::set<uint32_t> getGroupMembers(uint32_t gid);
    bool        isGroupMember(uint32_t groupId, uint32_t userId);

    //=====================离线消息=====================
    //msg应该是已经包装好的，可以直接用send发送
    void        addOfflineMsg(uint32_t uid, const std::string& msg);
    std::vector<std::string>
                getOfflineMsgs(uint32_t uid);

private:
    EntityManager()= default;
    ~EntityManager()= default;

    bool        loadAllUsers();
    bool        loadAllGroups();

    UserMap             m_users;        //存储所有用户的信息
    AccountUidMap       m_account2Uid;  //存储用户账号->uid的映射，为了实现按account查找
    uint32_t            m_maxUserId;    //当前最大的用户ID，新添加用户的时候用这个来分配ID
    uint32_t            m_maxFriendGroupId; //当前最大的用户ID，新添加分组的时候用这个来分配ID

    GroupMap            m_groups;       //存储所有群组的信息
    uint32_t            m_maxGroupId;   //当前最大的群组ID，新添加的群组用这个来分配ID

    //存储离线消息
    std::unordered_map<uint32_t , std::vector<std::string>>   m_offlineMsgs;

    RedisPtr            m_redis;        //redis实例
};


#endif //KYIMSERVER_ENTITYMANAGER_H
