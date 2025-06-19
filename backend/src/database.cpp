#include "database.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>

Database::Database(const std::string& dbPath) : dbPath_(dbPath), db_(nullptr), initialized_(false) {
}

Database::~Database() {
    if (db_) {
        sqlite3_close(db_);
    }
}

bool Database::initialize() {
    if (initialized_) return true;
    
    int rc = sqlite3_open(dbPath_.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to open database: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    // Enable foreign keys
    sqlite3_exec(db_, "PRAGMA foreign_keys = ON", nullptr, nullptr, nullptr);
    
    if (!createTables()) {
        std::cerr << "Failed to create tables" << std::endl;
        return false;
    }
    
    if (!createIndexes()) {
        std::cerr << "Failed to create indexes" << std::endl;
        return false;
    }
    
    initialized_ = true;
    std::cout << "Database initialized successfully" << std::endl;
    return true;
}

bool Database::createTables() {
    const char* createUsersTable = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            email TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            public_key TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            is_online BOOLEAN DEFAULT FALSE
        )
    )";
    
    const char* createGroupsTable = R"(
        CREATE TABLE IF NOT EXISTS groups (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            description TEXT,
            creator_id INTEGER NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (creator_id) REFERENCES users (id)
        )
    )";
    
    const char* createMessagesTable = R"(
        CREATE TABLE IF NOT EXISTS messages (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            sender_id INTEGER NOT NULL,
            receiver_id INTEGER,
            group_id INTEGER,
            content TEXT,
            encrypted_content TEXT,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            is_read BOOLEAN DEFAULT FALSE,
            message_type TEXT DEFAULT 'text',
            FOREIGN KEY (sender_id) REFERENCES users (id),
            FOREIGN KEY (receiver_id) REFERENCES users (id),
            FOREIGN KEY (group_id) REFERENCES groups (id)
        )
    )";
    
    const char* createGroupMembersTable = R"(
        CREATE TABLE IF NOT EXISTS group_members (
            group_id INTEGER NOT NULL,
            user_id INTEGER NOT NULL,
            role TEXT DEFAULT 'member',
            joined_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            PRIMARY KEY (group_id, user_id),
            FOREIGN KEY (group_id) REFERENCES groups (id),
            FOREIGN KEY (user_id) REFERENCES users (id)
        )
    )";
    
    const char* createSessionsTable = R"(
        CREATE TABLE IF NOT EXISTS sessions (
            token TEXT PRIMARY KEY,
            user_id INTEGER NOT NULL,
            expires_at DATETIME NOT NULL,
            FOREIGN KEY (user_id) REFERENCES users (id)
        )
    )";
    
    char* errMsg = nullptr;
    
    if (sqlite3_exec(db_, createUsersTable, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Failed to create users table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    
    if (sqlite3_exec(db_, createGroupsTable, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Failed to create groups table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    
    if (sqlite3_exec(db_, createMessagesTable, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Failed to create messages table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    
    if (sqlite3_exec(db_, createGroupMembersTable, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Failed to create group_members table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    
    if (sqlite3_exec(db_, createSessionsTable, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Failed to create sessions table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    
    return true;
}

bool Database::createIndexes() {
    const char* indexes[] = {
        "CREATE INDEX IF NOT EXISTS idx_users_username ON users(username)",
        "CREATE INDEX IF NOT EXISTS idx_users_email ON users(email)",
        "CREATE INDEX IF NOT EXISTS idx_messages_sender ON messages(sender_id)",
        "CREATE INDEX IF NOT EXISTS idx_messages_receiver ON messages(receiver_id)",
        "CREATE INDEX IF NOT EXISTS idx_messages_group ON messages(group_id)",
        "CREATE INDEX IF NOT EXISTS idx_messages_timestamp ON messages(timestamp)",
        "CREATE INDEX IF NOT EXISTS idx_group_members_group ON group_members(group_id)",
        "CREATE INDEX IF NOT EXISTS idx_group_members_user ON group_members(user_id)",
        "CREATE INDEX IF NOT EXISTS idx_sessions_token ON sessions(token)",
        "CREATE INDEX IF NOT EXISTS idx_sessions_user ON sessions(user_id)"
    };
    
    char* errMsg = nullptr;
    for (const char* index : indexes) {
        if (sqlite3_exec(db_, index, nullptr, nullptr, &errMsg) != SQLITE_OK) {
            std::cerr << "Failed to create index: " << errMsg << std::endl;
            sqlite3_free(errMsg);
            return false;
        }
    }
    
    return true;
}

bool Database::createUser(const std::string& username, const std::string& email, 
                         const std::string& passwordHash, const std::string& publicKey) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    const char* sql = "INSERT INTO users (username, email, password_hash, public_key) VALUES (?, ?, ?, ?)";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, email.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, passwordHash.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, publicKey.c_str(), -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

User Database::getUserByUsername(const std::string& username) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    const char* sql = "SELECT id, username, email, password_hash, public_key, created_at, is_online FROM users WHERE username = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return User{};
    }
    
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    
    User user;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user.id = sqlite3_column_int(stmt, 0);
        user.username = (const char*)sqlite3_column_text(stmt, 1);
        user.email = (const char*)sqlite3_column_text(stmt, 2);
        user.password_hash = (const char*)sqlite3_column_text(stmt, 3);
        user.public_key = (const char*)sqlite3_column_text(stmt, 4);
        user.created_at = (const char*)sqlite3_column_text(stmt, 5);
        user.is_online = sqlite3_column_int(stmt, 6) != 0;
    }
    
    sqlite3_finalize(stmt);
    return user;
}

User Database::getUserById(int id) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    const char* sql = "SELECT id, username, email, password_hash, public_key, created_at, is_online FROM users WHERE id = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return User{};
    }
    
    sqlite3_bind_int(stmt, 1, id);
    
    User user;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user.id = sqlite3_column_int(stmt, 0);
        user.username = (const char*)sqlite3_column_text(stmt, 1);
        user.email = (const char*)sqlite3_column_text(stmt, 2);
        user.password_hash = (const char*)sqlite3_column_text(stmt, 3);
        user.public_key = (const char*)sqlite3_column_text(stmt, 4);
        user.created_at = (const char*)sqlite3_column_text(stmt, 5);
        user.is_online = sqlite3_column_int(stmt, 6) != 0;
    }
    
    sqlite3_finalize(stmt);
    return user;
}

bool Database::updateUserOnlineStatus(int userId, bool online) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    const char* sql = "UPDATE users SET is_online = ? WHERE id = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, online ? 1 : 0);
    sqlite3_bind_int(stmt, 2, userId);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

std::vector<User> Database::getAllUsers() {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    const char* sql = "SELECT id, username, email, password_hash, public_key, created_at, is_online FROM users";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return {};
    }
    
    std::vector<User> users;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        User user;
        user.id = sqlite3_column_int(stmt, 0);
        user.username = (const char*)sqlite3_column_text(stmt, 1);
        user.email = (const char*)sqlite3_column_text(stmt, 2);
        user.password_hash = (const char*)sqlite3_column_text(stmt, 3);
        user.public_key = (const char*)sqlite3_column_text(stmt, 4);
        user.created_at = (const char*)sqlite3_column_text(stmt, 5);
        user.is_online = sqlite3_column_int(stmt, 6) != 0;
        users.push_back(user);
    }
    
    sqlite3_finalize(stmt);
    return users;
}

bool Database::saveMessage(const Message& message) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    const char* sql = "INSERT INTO messages (sender_id, receiver_id, group_id, content, encrypted_content, message_type) VALUES (?, ?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, message.sender_id);
    sqlite3_bind_int(stmt, 2, message.receiver_id);
    sqlite3_bind_int(stmt, 3, message.group_id);
    sqlite3_bind_text(stmt, 4, message.content.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, message.encrypted_content.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, message.message_type.c_str(), -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

std::vector<Message> Database::getMessages(int userId, int otherUserId, int limit) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    const char* sql = "SELECT id, sender_id, receiver_id, group_id, content, encrypted_content, timestamp, is_read, message_type FROM messages WHERE (sender_id = ? AND receiver_id = ?) OR (sender_id = ? AND receiver_id = ?) ORDER BY timestamp DESC LIMIT ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return {};
    }
    
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_int(stmt, 2, otherUserId);
    sqlite3_bind_int(stmt, 3, otherUserId);
    sqlite3_bind_int(stmt, 4, userId);
    sqlite3_bind_int(stmt, 5, limit);
    
    std::vector<Message> messages;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Message message;
        message.id = sqlite3_column_int(stmt, 0);
        message.sender_id = sqlite3_column_int(stmt, 1);
        message.receiver_id = sqlite3_column_int(stmt, 2);
        message.group_id = sqlite3_column_int(stmt, 3);
        message.content = (const char*)sqlite3_column_text(stmt, 4);
        message.encrypted_content = (const char*)sqlite3_column_text(stmt, 5);
        message.timestamp = (const char*)sqlite3_column_text(stmt, 6);
        message.is_read = sqlite3_column_int(stmt, 7) != 0;
        message.message_type = (const char*)sqlite3_column_text(stmt, 8);
        messages.push_back(message);
    }
    
    sqlite3_finalize(stmt);
    return messages;
}

std::vector<Message> Database::getGroupMessages(int groupId, int limit) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    const char* sql = "SELECT id, sender_id, receiver_id, group_id, content, encrypted_content, timestamp, is_read, message_type FROM messages WHERE group_id = ? ORDER BY timestamp DESC LIMIT ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return {};
    }
    
    sqlite3_bind_int(stmt, 1, groupId);
    sqlite3_bind_int(stmt, 2, limit);
    
    std::vector<Message> messages;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Message message;
        message.id = sqlite3_column_int(stmt, 0);
        message.sender_id = sqlite3_column_int(stmt, 1);
        message.receiver_id = sqlite3_column_int(stmt, 2);
        message.group_id = sqlite3_column_int(stmt, 3);
        message.content = (const char*)sqlite3_column_text(stmt, 4);
        message.encrypted_content = (const char*)sqlite3_column_text(stmt, 5);
        message.timestamp = (const char*)sqlite3_column_text(stmt, 6);
        message.is_read = sqlite3_column_int(stmt, 7) != 0;
        message.message_type = (const char*)sqlite3_column_text(stmt, 8);
        messages.push_back(message);
    }
    
    sqlite3_finalize(stmt);
    return messages;
}

bool Database::markMessageAsRead(int messageId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    const char* sql = "UPDATE messages SET is_read = TRUE WHERE id = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, messageId);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

bool Database::deleteMessage(int messageId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    const char* sql = "DELETE FROM messages WHERE id = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, messageId);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

bool Database::createGroup(const std::string& name, const std::string& description, int creatorId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    const char* sql = "INSERT INTO groups (name, description, creator_id) VALUES (?, ?, ?)";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, description.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, creatorId);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

bool Database::addUserToGroup(int groupId, int userId, const std::string& role) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    const char* sql = "INSERT OR REPLACE INTO group_members (group_id, user_id, role) VALUES (?, ?, ?)";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, groupId);
    sqlite3_bind_int(stmt, 2, userId);
    sqlite3_bind_text(stmt, 3, role.c_str(), -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

bool Database::removeUserFromGroup(int groupId, int userId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    const char* sql = "DELETE FROM group_members WHERE group_id = ? AND user_id = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, groupId);
    sqlite3_bind_int(stmt, 2, userId);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

std::vector<Group> Database::getUserGroups(int userId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    const char* sql = "SELECT g.id, g.name, g.description, g.creator_id, g.created_at FROM groups g JOIN group_members gm ON g.id = gm.group_id WHERE gm.user_id = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return {};
    }
    
    sqlite3_bind_int(stmt, 1, userId);
    
    std::vector<Group> groups;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Group group;
        group.id = sqlite3_column_int(stmt, 0);
        group.name = (const char*)sqlite3_column_text(stmt, 1);
        group.description = (const char*)sqlite3_column_text(stmt, 2);
        group.creator_id = sqlite3_column_int(stmt, 3);
        group.created_at = (const char*)sqlite3_column_text(stmt, 4);
        groups.push_back(group);
    }
    
    sqlite3_finalize(stmt);
    return groups;
}

std::vector<User> Database::getGroupMembers(int groupId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    const char* sql = "SELECT u.id, u.username, u.email, u.password_hash, u.public_key, u.created_at, u.is_online FROM users u JOIN group_members gm ON u.id = gm.user_id WHERE gm.group_id = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return {};
    }
    
    sqlite3_bind_int(stmt, 1, groupId);
    
    std::vector<User> users;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        User user;
        user.id = sqlite3_column_int(stmt, 0);
        user.username = (const char*)sqlite3_column_text(stmt, 1);
        user.email = (const char*)sqlite3_column_text(stmt, 2);
        user.password_hash = (const char*)sqlite3_column_text(stmt, 3);
        user.public_key = (const char*)sqlite3_column_text(stmt, 4);
        user.created_at = (const char*)sqlite3_column_text(stmt, 5);
        user.is_online = sqlite3_column_int(stmt, 6) != 0;
        users.push_back(user);
    }
    
    sqlite3_finalize(stmt);
    return users;
}

bool Database::saveSession(const std::string& token, int userId, const std::string& expiresAt) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    const char* sql = "INSERT OR REPLACE INTO sessions (token, user_id, expires_at) VALUES (?, ?, ?)";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, userId);
    sqlite3_bind_text(stmt, 3, expiresAt.c_str(), -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

int Database::getUserIdFromSession(const std::string& token) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    const char* sql = "SELECT user_id FROM sessions WHERE token = ? AND expires_at > datetime('now')";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_STATIC);
    
    int userId = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        userId = sqlite3_column_int(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    return userId;
}

bool Database::deleteSession(const std::string& token) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    const char* sql = "DELETE FROM sessions WHERE token = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

std::string Database::encryptData(const std::string& data) {
    // TODO: Implement actual encryption
    return data;
}

std::string Database::decryptData(const std::string& encryptedData) {
    // TODO: Implement actual decryption
    return encryptedData;
} 