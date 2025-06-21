#pragma once

#include <string>
#include <vector>
#include <memory>
#include "database.h"

class GroupChat {
public:
    GroupChat(std::shared_ptr<Database> database);
    
    // Group management
    bool createGroup(const std::string& name, const std::string& description, int creatorId);
    bool deleteGroup(int groupId, int userId);
    bool updateGroup(int groupId, const std::string& name, const std::string& description, int userId);
    
    // Member management
    bool addMember(int groupId, int userId, const std::string& role = "member");
    bool removeMember(int groupId, int userId, int adminId);
    bool updateMemberRole(int groupId, int userId, const std::string& role, int adminId);
    bool leaveGroup(int groupId, int userId);
    
    // Group queries
    std::vector<Group> getUserGroups(int userId);
    std::vector<User> getGroupMembers(int groupId);
    Group getGroupById(int groupId);
    
    // Permissions
    bool isGroupAdmin(int groupId, int userId);
    bool isGroupMember(int groupId, int userId);
    bool canManageGroup(int groupId, int userId);

private:
    std::shared_ptr<Database> database_;
}; 