//
// Created by gjk on 2020/2/10.
//

#ifndef KYIMSERVER_USERMANAGER_H
#define KYIMSERVER_USERMANAGER_H

#include <unordered_map>
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
};
typedef std::shared_ptr<User> UserPtr;

class UserManager : public noncopyable{
public:
    typedef std::unordered_map<uint32_t , UserPtr> UserMap;
    typedef std::unordered_map<std::string, uint32_t > AccountUidMap;

    static UserManager& getInstance(){
        static UserManager instance;
        return instance;
    }
    //初始化UserManage，应该在第一次使用UserManage实例之前就调用这个
    bool        init(const std::string& redishost, int redisport,
            const std::string& redispass);
    //添加一个新用户，返回这个用户的uid，0无效
    int         addNewUser(const UserPtr& user);
    UserPtr     getUserByUid(uint32_t uid);
    //这里的更新指的是将数据更新到数据库中
    bool        updateUserInfo(uint32_t uid);
    bool        updateUserPassowrd(uint32_t uid);
    UserPtr     getUserByAccount(const std::string& account);
    std::vector<uint32_t> getFriendListById(uint32_t uid);
    //获取好友列表


private:
    UserManager()= default;
    ~UserManager()= default;

    bool        loadAllUsers();

    UserMap             m_users;        //存储所有用户的信息
    AccountUidMap       m_account2Uid;  //存储用户账号->uid的映射，为了实现按account查找
    RedisPtr            m_redis;        //redis实例
    std::set<uint32_t > m_setAllUserId; //所有用户的uid集合
    uint32_t            m_allUserNumber;//当前总的用户量，新添加用户的时候用这个来分配ID
};


#endif //KYIMSERVER_USERMANAGER_H
