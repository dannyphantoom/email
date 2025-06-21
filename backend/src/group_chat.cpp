#include "group_chat.h"
#include <iostream>

GroupChat::GroupChat(std::shared_ptr<Database> database) : database_(database) {
}

bool GroupChat::createGroup(const std::string& name, const std::string& description, int creatorId) {
    // Create the group and get its ID
    int groupId = database_->createGroup(name, description, creatorId);
    
    if (groupId == -1) {
        std::cerr << "Failed to create group" << std::endl;
        return false;
    }
    
    // Add the creator as an admin member of the group
    return database_->addUserToGroup(groupId, creatorId, "admin");
}

bool GroupChat::deleteGroup(int groupId, int userId) {
    // Check if user is admin of the group
    if (!database_->isGroupAdmin(groupId, userId)) {
        return false;
    }
    return database_->deleteGroup(groupId);
}

bool GroupChat::updateGroup(int groupId, const std::string& name, const std::string& description, int userId) {
    // Check if user is admin of the group
    if (!database_->isGroupAdmin(groupId, userId)) {
        return false;
    }
    return database_->updateGroup(groupId, name, description);
}

bool GroupChat::addMember(int groupId, int userId, const std::string& role) {
    return database_->addUserToGroup(groupId, userId, role);
}

bool GroupChat::removeMember(int groupId, int userId, int adminId) {
    // Check if adminId is admin of the group
    if (!database_->isGroupAdmin(groupId, adminId)) {
        return false;
    }
    return database_->removeUserFromGroup(groupId, userId);
}

bool GroupChat::updateMemberRole(int groupId, int userId, const std::string& role, int adminId) {
    // Check if adminId is admin of the group
    if (!database_->isGroupAdmin(groupId, adminId)) {
        return false;
    }
    return database_->updateMemberRole(groupId, userId, role);
}

bool GroupChat::leaveGroup(int groupId, int userId) {
    // Check if user is a member of the group
    if (!database_->isGroupMember(groupId, userId)) {
        std::cerr << "User " << userId << " is not a member of group " << groupId << std::endl;
        return false;
    }
    
    // Check if user is admin
    bool isAdmin = database_->isGroupAdmin(groupId, userId);
    
    // Remove user from group
    bool removed = database_->removeUserFromGroup(groupId, userId);
    if (!removed) {
        std::cerr << "Failed to remove user " << userId << " from group " << groupId << std::endl;
        return false;
    }
    
    // If user was admin, check if there are other members and promote one to admin
    if (isAdmin) {
        auto members = database_->getGroupMembers(groupId);
        if (members.size() > 0) {
            // Promote the first remaining member to admin
            int newAdminId = members[0].id;
            bool promoted = database_->updateMemberRole(groupId, newAdminId, "admin");
            if (promoted) {
                std::cout << "Promoted user " << newAdminId << " to admin of group " << groupId << std::endl;
            } else {
                std::cerr << "Failed to promote user " << newAdminId << " to admin of group " << groupId << std::endl;
            }
        } else {
            // No members left, delete the group
            std::cout << "No members left in group " << groupId << ", deleting group" << std::endl;
            bool deleted = database_->deleteGroup(groupId);
            if (deleted) {
                std::cout << "Group " << groupId << " deleted successfully" << std::endl;
            } else {
                std::cerr << "Failed to delete group " << groupId << std::endl;
            }
        }
    }
    
    return true;
}

std::vector<Group> GroupChat::getUserGroups(int userId) {
    return database_->getUserGroups(userId);
}

std::vector<User> GroupChat::getGroupMembers(int groupId) {
    return database_->getGroupMembers(groupId);
}

Group GroupChat::getGroupById(int groupId) {
    return database_->getGroupById(groupId);
}

bool GroupChat::isGroupAdmin(int groupId, int userId) {
    return database_->isGroupAdmin(groupId, userId);
}

bool GroupChat::isGroupMember(int groupId, int userId) {
    return database_->isGroupMember(groupId, userId);
}

bool GroupChat::canManageGroup(int groupId, int userId) {
    return database_->isGroupAdmin(groupId, userId);
} 