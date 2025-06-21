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