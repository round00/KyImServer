//
// Created by gjk on 2020/2/14.
//

#include "FriendCroup.h"

const char* CFriendGroup::DEFAULT_NAME = "My Friends";
void CFriendGroup::addUsers(const std::unordered_set<int> &uids) {
    m_userIds.insert(uids.begin(), uids.end());
}

bool CFriendGroup::existsUser(uint32_t uid) {
    return m_userIds.find(uid) != m_userIds.end();
}