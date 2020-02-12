//
// Created by gjk on 2020/2/10.
//

#ifndef KYIMSERVER_USERMANAGER_H
#define KYIMSERVER_USERMANAGER_H

#include <unordered_map>
#include <string>
#include <set>
#include <memory>
#include "Redis.h"
#include "noncopyable.h"

enum USER_STATE{
    USER_STATE_OFFLINE = 0,
    USER_STATE_ONLINE
};

struct User{
    User():m_userId(0),m_userState(USER_STATE_OFFLINE){}
    explicit User(const std::vector<std::pair<std::string, std::string>>& userinfo);


    int             m_userId;
    std::string     m_userName;
    std::string     m_userPassword;
    std::string     m_userAccount;
    std::string     m_userPhoneNum;
    USER_STATE      m_userState;
};
typedef std::shared_ptr<User> UserPtr;

class UserManager : public noncopyable{
public:
    typedef std::unordered_map<int, UserPtr> UserMap;
    typedef std::unordered_map<std::string, int> AccountUidMap;

    static UserManager& getInstance(){
        static UserManager instance;
        return instance;
    }
    //初始化UserManage，应该在第一次使用UserManage实例之前就调用这个
    bool        init(const std::string& redishost, int redisport,
            const std::string& redispass);
    //添加一个新用户，返回这个用户的uid，0无效
    int         addNewUser(const UserPtr& user);
    UserPtr     getUserByUid(int uid);
    UserPtr     getUserByAccount(const std::string& account);

private:
    UserManager()= default;
    ~UserManager()= default;

    bool        loadAllUsers();

    UserMap             m_users;        //存储所有用户的信息
    AccountUidMap       m_account2Uid;  //存储用户账号->uid的映射，为了实现按account查找
    RedisPtr            m_redis;        //redis实例
    std::set<int>       m_setAllUserId; //所有用户的uid集合
    int                 m_allUserNumber;//当前总的用户量，新添加用户的时候用这个来分配ID
};


#endif //KYIMSERVER_USERMANAGER_H
