//
// Created by gjk on 2020/2/14.
//

#ifndef KYIMSERVER_FRIENDCROUP_H
#define KYIMSERVER_FRIENDCROUP_H

#include <unordered_set>
#include <string>
#include <stdint.h>

class CFriendGroup{
public:
    CFriendGroup(uint32_t id, const std::string& name=DEFAULT_NAME,
            const std::unordered_set<int>& uids = std::unordered_set<int>()):
    m_id(id), m_name(name), m_userIds(uids){}
    //注册用户的时候生成一个默认的用户分组
    static const char* DEFAULT_NAME;

    void            addUser(uint32_t uid){m_userIds.insert(uid);}
    void            addUsers(const std::unordered_set<int>& uids);
    void            delUser(uint32_t uid){m_userIds.erase(uid);}
    void            delAllUser(){m_userIds.clear();}
    bool            existsUser(uint32_t uid);
    std::unordered_set<int>   getUserIds(){return m_userIds;}

    uint32_t        getId(){return m_id;}

    std::string     getName(){return m_name;}
    void            setName(const std::string& name){m_name = name;}

private:
    uint32_t        m_id;
    std::string     m_name;
    std::unordered_set<int>   m_userIds;
};

const char* CFriendGroup::DEFAULT_NAME = "My Friends";
#endif //KYIMSERVER_FRIENDCROUP_H
