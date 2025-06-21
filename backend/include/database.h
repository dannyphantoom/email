#pragma once

#include <string>
#include <vector>
#include <memory>
#include <sqlite3.h>
#include <mutex>

struct User {
    int id;
    std::string username;
    std::string email;
    std::string password_hash;
    std::string public_key;
    std::string created_at;
    bool is_online;
};

struct Message {
    int id;
    int sender_id;
    int receiver_id;
    int group_id;  // 0 if direct message
    std::string content;
    std::string encrypted_content;
    std::string timestamp;
    bool is_read;
    std::string message_type; // "text", "file", "image"
};

struct Group {
    int id;
    std::string name;
    std::string description;
    int creator_id;
    std::string created_at;
};

struct GroupMember {
    int group_id;
    int user_id;
    std::string role; // "admin", "member"
    std::string joined_at;
};

struct ChatBackup {
    int id;
    int user_id;
    std::string backup_name;
    std::string backup_data; // JSON string containing chat data
    std::string created_at;
    std::string description;
};

struct ChatSession {
    int id;
    int user_id;
    int other_user_id; // For direct messages
    int group_id;      // For group chats
    std::string last_message;
    std::string last_timestamp;
    int unread_count;
    std::string updated_at;
};

class Database {
public:
    Database(const std::string& dbPath = "cockpit.db");
    ~Database();

    bool initialize();
    bool isInitialized() const { return initialized_; }

    // User operations
    bool createUser(const std::string& username, const std::string& email, 
                   const std::string& passwordHash, const std::string& publicKey);
    User getUserByUsername(const std::string& username);
    User getUserById(int id);
    bool updateUserOnlineStatus(int userId, bool online);
    std::vector<User> getAllUsers();

    // Message operations
    bool saveMessage(const Message& message);
    std::vector<Message> getMessages(int userId, int otherUserId, int limit = 50);
    std::vector<Message> getGroupMessages(int groupId, int limit = 50);
    bool markMessageAsRead(int messageId);
    bool deleteMessage(int messageId);

    // Group operations
    bool createGroup(const std::string& name, const std::string& description, int creatorId);
    bool addUserToGroup(int groupId, int userId, const std::string& role = "member");
    bool removeUserFromGroup(int groupId, int userId);
    std::vector<Group> getUserGroups(int userId);
    std::vector<User> getGroupMembers(int groupId);
    Group getGroupById(int groupId);
    bool updateGroup(int groupId, const std::string& name, const std::string& description);
    bool deleteGroup(int groupId);
    bool isGroupMember(int groupId, int userId);
    bool isGroupAdmin(int groupId, int userId);
    bool updateMemberRole(int groupId, int userId, const std::string& role);

    // Chat session operations
    bool createOrUpdateChatSession(int userId, int otherUserId, int groupId, 
                                  const std::string& lastMessage, int unreadCount = 0);
    std::vector<ChatSession> getUserChatSessions(int userId);
    bool updateChatSessionUnreadCount(int sessionId, int unreadCount);
    bool deleteChatSession(int sessionId);

    // Backup operations
    bool createChatBackup(int userId, const std::string& backupName, 
                         const std::string& backupData, const std::string& description = "");
    std::vector<ChatBackup> getUserBackups(int userId);
    ChatBackup getBackupById(int backupId);
    bool deleteBackup(int backupId, int userId);
    bool restoreFromBackup(int backupId, int userId);

    // Session management
    bool saveSession(const std::string& token, int userId, const std::string& expiresAt);
    int getUserIdFromSession(const std::string& token);
    bool deleteSession(const std::string& token);

private:
    std::string dbPath_;
    sqlite3* db_;
    bool initialized_;
    std::mutex dbMutex_;

    bool createTables();
    bool createIndexes();
    std::string encryptData(const std::string& data);
    std::string decryptData(const std::string& encryptedData);
}; 