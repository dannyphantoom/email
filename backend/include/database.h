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