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
    
    std::cout << "[DEBUG] Database::initialize() - Opening database: " << dbPath_ << std::endl;
    
    int rc = sqlite3_open(dbPath_.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to open database: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    std::cout << "[DEBUG] Database opened successfully" << std::endl;
    
    // Enable foreign keys
    sqlite3_exec(db_, "PRAGMA foreign_keys = ON", nullptr, nullptr, nullptr);
    
    std::cout << "[DEBUG] Creating tables..." << std::endl;
    if (!createTables()) {
        std::cerr << "Failed to create tables" << std::endl;
        return false;
    }
    
    std::cout << "[DEBUG] Creating indexes..." << std::endl;
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
            file_name TEXT,
            file_path TEXT,
            file_size INTEGER,
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
    
    const char* createChatSessionsTable = R"(
        CREATE TABLE IF NOT EXISTS chat_sessions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            other_user_id INTEGER,
            group_id INTEGER,
            last_message TEXT,
            last_timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            unread_count INTEGER DEFAULT 0,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (user_id) REFERENCES users (id),
            FOREIGN KEY (other_user_id) REFERENCES users (id),
            FOREIGN KEY (group_id) REFERENCES groups (id)
        )
    )";
    
    const char* createChatBackupsTable = R"(
        CREATE TABLE IF NOT EXISTS chat_backups (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            backup_name TEXT NOT NULL,
            backup_data TEXT NOT NULL,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            description TEXT,
            FOREIGN KEY (user_id) REFERENCES users (id)
        )
    )";
    
    const char* createGroupInvitationsTable = R"(
        CREATE TABLE IF NOT EXISTS group_invitations (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            group_id INTEGER NOT NULL,
            inviter_id INTEGER NOT NULL,
            invitee_id INTEGER NOT NULL,
            role TEXT DEFAULT 'member',
            status TEXT DEFAULT 'pending',
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            expires_at DATETIME DEFAULT (datetime('now', '+30 minutes')),
            responded_at DATETIME,
            FOREIGN KEY (group_id) REFERENCES groups (id),
            FOREIGN KEY (inviter_id) REFERENCES users (id),
            FOREIGN KEY (invitee_id) REFERENCES users (id)
        )
    )";
    
    char* errMsg = nullptr;
    
    if (sqlite3_exec(db_, createUsersTable, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Failed to create users table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    std::cout << "[DEBUG] Users table created successfully" << std::endl;
    
    if (sqlite3_exec(db_, createGroupsTable, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Failed to create groups table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    std::cout << "[DEBUG] Groups table created successfully" << std::endl;
    
    if (sqlite3_exec(db_, createMessagesTable, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Failed to create messages table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    std::cout << "[DEBUG] Messages table created successfully" << std::endl;
    
    if (sqlite3_exec(db_, createGroupMembersTable, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Failed to create group_members table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    std::cout << "[DEBUG] Group_members table created successfully" << std::endl;
    
    if (sqlite3_exec(db_, createSessionsTable, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Failed to create sessions table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    std::cout << "[DEBUG] Sessions table created successfully" << std::endl;
    
    if (sqlite3_exec(db_, createChatSessionsTable, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Failed to create chat_sessions table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    std::cout << "[DEBUG] Chat_sessions table created successfully" << std::endl;
    
    if (sqlite3_exec(db_, createChatBackupsTable, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Failed to create chat_backups table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    std::cout << "[DEBUG] Chat_backups table created successfully" << std::endl;
    
    if (sqlite3_exec(db_, createGroupInvitationsTable, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Failed to create group_invitations table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    std::cout << "[DEBUG] Group_invitations table created successfully" << std::endl;
    
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
        "CREATE INDEX IF NOT EXISTS idx_sessions_user ON sessions(user_id)",
        "CREATE INDEX IF NOT EXISTS idx_chat_sessions_user ON chat_sessions(user_id)",
        "CREATE INDEX IF NOT EXISTS idx_chat_sessions_other_user ON chat_sessions(other_user_id)",
        "CREATE INDEX IF NOT EXISTS idx_chat_sessions_group ON chat_sessions(group_id)",
        "CREATE INDEX IF NOT EXISTS idx_chat_sessions_updated ON chat_sessions(updated_at)",
        "CREATE INDEX IF NOT EXISTS idx_chat_backups_user ON chat_backups(user_id)",
        "CREATE INDEX IF NOT EXISTS idx_chat_backups_created ON chat_backups(created_at)"
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
    if (rc != SQLITE_DONE) {
        std::cerr << "Failed to insert user: " << sqlite3_errmsg(db_) << std::endl;
    }
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

User Database::getUserByUsername(const std::string& username) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    std::cerr << "[DEBUG] getUserByUsername called with username: '" << username << "'" << std::endl;
    
    const char* sql = "SELECT id, username, email, password_hash, public_key, created_at, is_online FROM users WHERE username = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return User{};
    }
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    std::cerr << "[DEBUG] getUserByUsername: bound username parameter: '" << username << "'" << std::endl;
    
    User user{};
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user.id = sqlite3_column_int(stmt, 0);
        user.username = (const char*)sqlite3_column_text(stmt, 1);
        user.email = (const char*)sqlite3_column_text(stmt, 2);
        user.password_hash = (const char*)sqlite3_column_text(stmt, 3);
        user.public_key = (const char*)sqlite3_column_text(stmt, 4);
        user.created_at = (const char*)sqlite3_column_text(stmt, 5);
        user.is_online = sqlite3_column_int(stmt, 6) != 0;
        std::cerr << "[DEBUG] getUserByUsername found: id=" << user.id << ", username='" << user.username << "', email='" << user.email << "'" << std::endl;
    } else {
        std::cerr << "[DEBUG] getUserByUsername: no user found for '" << username << "'" << std::endl;
    }
    sqlite3_finalize(stmt);
    return user;
}

User Database::getUserById(int id) {
    // Note: This function should not acquire dbMutex_ if called from within another function that already holds it
    // The caller is responsible for ensuring thread safety
    
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
    std::cout << "[DEBUG] saveMessage: Starting function" << std::endl;
    std::lock_guard<std::mutex> lock(dbMutex_);
    std::cout << "[DEBUG] saveMessage: Acquired database lock" << std::endl;
    
    std::cout << "[DEBUG] saveMessage called with:" << std::endl;
    std::cout << "  sender_id: " << message.sender_id << std::endl;
    std::cout << "  receiver_id: " << message.receiver_id << std::endl;
    std::cout << "  group_id: " << message.group_id << std::endl;
    std::cout << "  content: " << message.content << std::endl;
    std::cout << "  encrypted_content: " << message.encrypted_content << std::endl;
    std::cout << "  message_type: " << message.message_type << std::endl;
    
    // Validate sender exists
    std::cout << "[DEBUG] saveMessage: About to validate sender" << std::endl;
    User sender = getUserById(message.sender_id);
    std::cout << "[DEBUG] saveMessage: getUserById returned, sender.id = " << sender.id << std::endl;
    if (sender.id == 0) {
        std::cerr << "[DEBUG] Sender with ID " << message.sender_id << " does not exist" << std::endl;
        return false;
    }
    std::cout << "[DEBUG] Sender validation passed: " << sender.username << std::endl;
    
    // Validate group exists if it's a group message
    if (message.group_id > 0) {
        std::cout << "[DEBUG] saveMessage: About to validate group" << std::endl;
        Group group = getGroupById(message.group_id);
        std::cout << "[DEBUG] saveMessage: getGroupById returned, group.id = " << group.id << std::endl;
        if (group.id == 0) {
            std::cerr << "[DEBUG] Group with ID " << message.group_id << " does not exist" << std::endl;
            return false;
        }
        std::cout << "[DEBUG] Group validation passed: " << group.name << std::endl;
        
        // Check if sender is a member of the group
        std::cout << "[DEBUG] saveMessage: About to check group membership" << std::endl;
        bool isMember = isGroupMemberInternal(message.group_id, message.sender_id);
        std::cout << "[DEBUG] saveMessage: isGroupMember returned: " << (isMember ? "true" : "false") << std::endl;
        if (!isMember) {
            std::cerr << "[DEBUG] User " << message.sender_id << " is not a member of group " << message.group_id << std::endl;
            return false;
        }
        std::cout << "[DEBUG] Group membership validation passed" << std::endl;
    }
    
    // Validate receiver exists if it's a direct message
    if (message.receiver_id > 0) {
        std::cout << "[DEBUG] saveMessage: About to validate receiver" << std::endl;
        User receiver = getUserById(message.receiver_id);
        std::cout << "[DEBUG] saveMessage: getUserById for receiver returned, receiver.id = " << receiver.id << std::endl;
        if (receiver.id == 0) {
            std::cerr << "[DEBUG] Receiver with ID " << message.receiver_id << " does not exist" << std::endl;
            return false;
        }
        std::cout << "[DEBUG] Receiver validation passed: " << receiver.username << std::endl;
    }
    
    std::cout << "[DEBUG] saveMessage: About to prepare SQL statement" << std::endl;
    const char* sql = "INSERT INTO messages (sender_id, receiver_id, group_id, content, encrypted_content, message_type, file_name, file_path, file_size) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt;
    
    std::cout << "[DEBUG] saveMessage: Calling sqlite3_prepare_v2" << std::endl;
    int prep_result = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    std::cout << "[DEBUG] saveMessage: sqlite3_prepare_v2 returned: " << prep_result << std::endl;
    if (prep_result != SQLITE_OK) {
        std::cerr << "[DEBUG] Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    std::cout << "[DEBUG] saveMessage: SQL statement prepared successfully" << std::endl;
    
    std::cout << "[DEBUG] saveMessage: About to bind parameters" << std::endl;
    sqlite3_bind_int(stmt, 1, message.sender_id);
    
    // Handle NULL values for receiver_id when it's 0
    if (message.receiver_id > 0) {
        sqlite3_bind_int(stmt, 2, message.receiver_id);
    } else {
        sqlite3_bind_null(stmt, 2);
    }
    
    sqlite3_bind_int(stmt, 3, message.group_id);
    sqlite3_bind_text(stmt, 4, message.content.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, message.encrypted_content.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, message.message_type.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, message.file_name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 8, message.file_path.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 9, message.file_size);
    std::cout << "[DEBUG] saveMessage: Parameters bound successfully" << std::endl;
    
    std::cout << "[DEBUG] saveMessage: About to execute sqlite3_step" << std::endl;
    int rc = sqlite3_step(stmt);
    std::cout << "[DEBUG] saveMessage: SQL insert result: " << (rc == SQLITE_DONE ? "SUCCESS" : "FAILED") << std::endl;
    sqlite3_finalize(stmt);
    std::cout << "[DEBUG] saveMessage: Function ending" << std::endl;
    return rc == SQLITE_DONE;
}

std::vector<Message> Database::getMessages(int userId, int otherUserId, int limit) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    const char* sql = "SELECT id, sender_id, receiver_id, group_id, content, encrypted_content, timestamp, is_read, message_type FROM messages WHERE (sender_id = ? AND receiver_id = ?) OR (sender_id = ? AND receiver_id = ?) ORDER BY timestamp ASC LIMIT ?";
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
    
    const char* sql = "SELECT id, sender_id, receiver_id, group_id, content, encrypted_content, timestamp, is_read, message_type, file_name, file_path, file_size FROM messages WHERE group_id = ? ORDER BY timestamp ASC LIMIT ?";
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
        message.file_name = sqlite3_column_text(stmt, 9) ? (const char*)sqlite3_column_text(stmt, 9) : "";
        message.file_path = sqlite3_column_text(stmt, 10) ? (const char*)sqlite3_column_text(stmt, 10) : "";
        message.file_size = sqlite3_column_int(stmt, 11);
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
    
    if (rc == SQLITE_DONE) {
        return true;
    } else {
        return false;
    }
}

bool Database::addUserToGroup(int groupId, int userId, const std::string& role) {
    std::cout << "[DEBUG] addUserToGroup: ENTERING FUNCTION" << std::endl;
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    return addUserToGroupInternal(groupId, userId, role);
}

bool Database::addUserToGroupInternal(int groupId, int userId, const std::string& role) {
    std::cout << "[DEBUG] addUserToGroupInternal: groupId=" << groupId << ", userId=" << userId << ", role=" << role << std::endl;
    
    // Check if user already exists (internal call, no mutex needed)
    const char* userSql = "SELECT id, username FROM users WHERE id = ?";
    sqlite3_stmt* userStmt;
    if (sqlite3_prepare_v2(db_, userSql, -1, &userStmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare user statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    sqlite3_bind_int(userStmt, 1, userId);
    
    std::string username;
    if (sqlite3_step(userStmt) == SQLITE_ROW) {
        username = (const char*)sqlite3_column_text(userStmt, 1);
        std::cout << "[DEBUG] addUserToGroupInternal: Found user: " << username << std::endl;
    } else {
        std::cout << "[DEBUG] addUserToGroupInternal: User not found with id=" << userId << std::endl;
        sqlite3_finalize(userStmt);
        return false;
    }
    sqlite3_finalize(userStmt);
    
    // Check if group exists (internal call, no mutex needed)
    const char* groupSql = "SELECT id, name FROM groups WHERE id = ?";
    sqlite3_stmt* groupStmt;
    if (sqlite3_prepare_v2(db_, groupSql, -1, &groupStmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare group statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    sqlite3_bind_int(groupStmt, 1, groupId);
    
    std::string groupName;
    if (sqlite3_step(groupStmt) == SQLITE_ROW) {
        groupName = (const char*)sqlite3_column_text(groupStmt, 1);
        std::cout << "[DEBUG] addUserToGroupInternal: Found group: " << groupName << std::endl;
    } else {
        std::cout << "[DEBUG] addUserToGroupInternal: Group not found with id=" << groupId << std::endl;
        sqlite3_finalize(groupStmt);
        return false;
    }
    sqlite3_finalize(groupStmt);
    
    // Check if user is already a member
    if (isGroupMemberInternal(groupId, userId)) {
        std::cout << "[DEBUG] addUserToGroupInternal: User is already a member of this group" << std::endl;
        return true; // Already a member, consider this success
    }
    
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
    
    std::cout << "[DEBUG] addUserToGroupInternal: Insert result=" << rc << " (SQLITE_DONE=" << SQLITE_DONE << ")" << std::endl;
    
    if (rc != SQLITE_DONE) {
        std::cout << "[DEBUG] addUserToGroupInternal: Insert failed with error: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    std::cout << "[DEBUG] addUserToGroupInternal: SUCCESS - User added to group" << std::endl;
    return true;
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
    
    const char* sql = "SELECT u.id, u.username, u.email, u.password_hash, u.public_key, u.created_at, u.is_online, gm.role FROM users u JOIN group_members gm ON u.id = gm.user_id WHERE gm.group_id = ?";
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
        // Note: role is not part of User struct, so we'll need to handle it separately
        users.push_back(user);
    }
    
    sqlite3_finalize(stmt);
    return users;
}

std::vector<GroupMemberInfo> Database::getGroupMembersWithRole(int groupId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    const char* sql = "SELECT u.id, u.username, u.email, gm.role, u.is_online, u.created_at FROM users u JOIN group_members gm ON u.id = gm.user_id WHERE gm.group_id = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return {};
    }
    
    sqlite3_bind_int(stmt, 1, groupId);
    
    std::vector<GroupMemberInfo> members;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        GroupMemberInfo member;
        member.id = sqlite3_column_int(stmt, 0);
        member.username = (const char*)sqlite3_column_text(stmt, 1);
        member.email = (const char*)sqlite3_column_text(stmt, 2);
        member.role = (const char*)sqlite3_column_text(stmt, 3);
        member.is_online = sqlite3_column_int(stmt, 4) != 0;
        member.created_at = (const char*)sqlite3_column_text(stmt, 5);
        members.push_back(member);
    }
    
    sqlite3_finalize(stmt);
    return members;
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
    
    std::cerr << "[DEBUG] getUserIdFromSession called with token: '" << token << "'" << std::endl;
    
    const char* sql = "SELECT user_id FROM sessions WHERE token = ? AND expires_at > datetime('now')";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_STATIC);
    std::cerr << "[DEBUG] getUserIdFromSession: bound token parameter: '" << token << "'" << std::endl;
    
    int userId = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        userId = sqlite3_column_int(stmt, 0);
        std::cerr << "[DEBUG] getUserIdFromSession found: userId=" << userId << std::endl;
    } else {
        std::cerr << "[DEBUG] getUserIdFromSession: no session found for token: '" << token << "'" << std::endl;
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

// Group management methods
Group Database::getGroupById(int groupId) {
    // Note: This function should not acquire dbMutex_ if called from within another function that already holds it
    // The caller is responsible for ensuring thread safety
    
    const char* sql = "SELECT id, name, description, creator_id, created_at FROM groups WHERE id = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return Group{};
    }
    
    sqlite3_bind_int(stmt, 1, groupId);
    
    Group group{};
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        group.id = sqlite3_column_int(stmt, 0);
        group.name = (const char*)sqlite3_column_text(stmt, 1);
        group.description = (const char*)sqlite3_column_text(stmt, 2);
        group.creator_id = sqlite3_column_int(stmt, 3);
        group.created_at = (const char*)sqlite3_column_text(stmt, 4);
    }
    
    sqlite3_finalize(stmt);
    return group;
}

bool Database::updateGroup(int groupId, const std::string& name, const std::string& description) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    const char* sql = "UPDATE groups SET name = ?, description = ? WHERE id = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, description.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, groupId);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

bool Database::deleteGroup(int groupId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    // Start a transaction
    sqlite3_exec(db_, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);
    
    try {
        // Delete group messages
        const char* deleteMessagesSql = "DELETE FROM messages WHERE group_id = ?";
        sqlite3_stmt* messagesStmt;
        
        if (sqlite3_prepare_v2(db_, deleteMessagesSql, -1, &messagesStmt, nullptr) != SQLITE_OK) {
            std::cerr << "Failed to prepare delete messages statement: " << sqlite3_errmsg(db_) << std::endl;
            sqlite3_exec(db_, "ROLLBACK", nullptr, nullptr, nullptr);
            return false;
        }
        
        sqlite3_bind_int(messagesStmt, 1, groupId);
        int messagesRc = sqlite3_step(messagesStmt);
        sqlite3_finalize(messagesStmt);
        
        if (messagesRc != SQLITE_DONE) {
            std::cerr << "Failed to delete group messages" << std::endl;
            sqlite3_exec(db_, "ROLLBACK", nullptr, nullptr, nullptr);
            return false;
        }
        
        // Delete group members
        const char* deleteMembersSql = "DELETE FROM group_members WHERE group_id = ?";
        sqlite3_stmt* membersStmt;
        
        if (sqlite3_prepare_v2(db_, deleteMembersSql, -1, &membersStmt, nullptr) != SQLITE_OK) {
            std::cerr << "Failed to prepare delete members statement: " << sqlite3_errmsg(db_) << std::endl;
            sqlite3_exec(db_, "ROLLBACK", nullptr, nullptr, nullptr);
            return false;
        }
        
        sqlite3_bind_int(membersStmt, 1, groupId);
        int membersRc = sqlite3_step(membersStmt);
        sqlite3_finalize(membersStmt);
        
        if (membersRc != SQLITE_DONE) {
            std::cerr << "Failed to delete group members" << std::endl;
            sqlite3_exec(db_, "ROLLBACK", nullptr, nullptr, nullptr);
            return false;
        }
        
        // Delete the group itself
        const char* deleteGroupSql = "DELETE FROM groups WHERE id = ?";
        sqlite3_stmt* groupStmt;
        
        if (sqlite3_prepare_v2(db_, deleteGroupSql, -1, &groupStmt, nullptr) != SQLITE_OK) {
            std::cerr << "Failed to prepare delete group statement: " << sqlite3_errmsg(db_) << std::endl;
            sqlite3_exec(db_, "ROLLBACK", nullptr, nullptr, nullptr);
            return false;
        }
        
        sqlite3_bind_int(groupStmt, 1, groupId);
        int groupRc = sqlite3_step(groupStmt);
        sqlite3_finalize(groupStmt);
        
        if (groupRc != SQLITE_DONE) {
            std::cerr << "Failed to delete group" << std::endl;
            sqlite3_exec(db_, "ROLLBACK", nullptr, nullptr, nullptr);
            return false;
        }
        
        // Commit the transaction
        sqlite3_exec(db_, "COMMIT", nullptr, nullptr, nullptr);
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception in deleteGroup: " << e.what() << std::endl;
        sqlite3_exec(db_, "ROLLBACK", nullptr, nullptr, nullptr);
        return false;
    }
}

bool Database::isGroupMember(int groupId, int userId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    return isGroupMemberInternal(groupId, userId);
}

bool Database::isGroupMemberInternal(int groupId, int userId) {
    std::cout << "[DEBUG] isGroupMemberInternal: Starting function with groupId=" << groupId << ", userId=" << userId << std::endl;
    
    const char* sql = "SELECT COUNT(*) FROM group_members WHERE group_id = ? AND user_id = ?";
    sqlite3_stmt* stmt;
    
    std::cout << "[DEBUG] isGroupMemberInternal: About to prepare statement" << std::endl;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    std::cout << "[DEBUG] isGroupMemberInternal: Statement prepared successfully" << std::endl;
    
    sqlite3_bind_int(stmt, 1, groupId);
    sqlite3_bind_int(stmt, 2, userId);
    std::cout << "[DEBUG] isGroupMemberInternal: Parameters bound" << std::endl;
    
    bool isMember = false;
    std::cout << "[DEBUG] isGroupMemberInternal: About to execute query" << std::endl;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        isMember = sqlite3_column_int(stmt, 0) > 0;
        std::cout << "[DEBUG] isGroupMemberInternal: Query executed, count=" << sqlite3_column_int(stmt, 0) << ", isMember=" << (isMember ? "true" : "false") << std::endl;
    } else {
        std::cout << "[DEBUG] isGroupMemberInternal: Query executed, no rows returned" << std::endl;
    }
    
    sqlite3_finalize(stmt);
    std::cout << "[DEBUG] isGroupMemberInternal: Statement finalized, returning " << (isMember ? "true" : "false") << std::endl;
    return isMember;
}

bool Database::isGroupAdmin(int groupId, int userId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    const char* sql = "SELECT COUNT(*) FROM group_members WHERE group_id = ? AND user_id = ? AND role = 'admin'";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, groupId);
    sqlite3_bind_int(stmt, 2, userId);
    
    bool isAdmin = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        isAdmin = sqlite3_column_int(stmt, 0) > 0;
    }
    
    sqlite3_finalize(stmt);
    return isAdmin;
}

bool Database::updateMemberRole(int groupId, int userId, const std::string& role) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    const char* sql = "UPDATE group_members SET role = ? WHERE group_id = ? AND user_id = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, role.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, groupId);
    sqlite3_bind_int(stmt, 3, userId);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

// Chat session methods
bool Database::createOrUpdateChatSession(int userId, int otherUserId, int groupId, 
                                        const std::string& lastMessage, int unreadCount) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    const char* sql = "INSERT OR REPLACE INTO chat_sessions (user_id, other_user_id, group_id, last_message, unread_count, updated_at) VALUES (?, ?, ?, ?, ?, datetime('now'))";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_int(stmt, 2, otherUserId);
    sqlite3_bind_int(stmt, 3, groupId);
    sqlite3_bind_text(stmt, 4, lastMessage.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 5, unreadCount);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

std::vector<ChatSession> Database::getUserChatSessions(int userId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    const char* sql = "SELECT id, user_id, other_user_id, group_id, last_message, last_timestamp, unread_count, updated_at FROM chat_sessions WHERE user_id = ? ORDER BY updated_at DESC";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return {};
    }
    
    sqlite3_bind_int(stmt, 1, userId);
    
    std::vector<ChatSession> sessions;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ChatSession session;
        session.id = sqlite3_column_int(stmt, 0);
        session.user_id = sqlite3_column_int(stmt, 1);
        session.other_user_id = sqlite3_column_int(stmt, 2);
        session.group_id = sqlite3_column_int(stmt, 3);
        session.last_message = (const char*)sqlite3_column_text(stmt, 4);
        session.last_timestamp = (const char*)sqlite3_column_text(stmt, 5);
        session.unread_count = sqlite3_column_int(stmt, 6);
        session.updated_at = (const char*)sqlite3_column_text(stmt, 7);
        sessions.push_back(session);
    }
    
    sqlite3_finalize(stmt);
    return sessions;
}

bool Database::updateChatSessionUnreadCount(int sessionId, int unreadCount) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    const char* sql = "UPDATE chat_sessions SET unread_count = ? WHERE id = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, unreadCount);
    sqlite3_bind_int(stmt, 2, sessionId);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

bool Database::deleteChatSession(int sessionId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    const char* sql = "DELETE FROM chat_sessions WHERE id = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, sessionId);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

// Backup methods
bool Database::createChatBackup(int userId, const std::string& backupName, 
                               const std::string& backupData, const std::string& description) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    const char* sql = "INSERT INTO chat_backups (user_id, backup_name, backup_data, description) VALUES (?, ?, ?, ?)";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_text(stmt, 2, backupName.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, backupData.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, description.c_str(), -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

std::vector<ChatBackup> Database::getUserBackups(int userId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    const char* sql = "SELECT id, user_id, backup_name, backup_data, created_at, description FROM chat_backups WHERE user_id = ? ORDER BY created_at DESC";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return {};
    }
    
    sqlite3_bind_int(stmt, 1, userId);
    
    std::vector<ChatBackup> backups;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ChatBackup backup;
        backup.id = sqlite3_column_int(stmt, 0);
        backup.user_id = sqlite3_column_int(stmt, 1);
        backup.backup_name = (const char*)sqlite3_column_text(stmt, 2);
        backup.backup_data = (const char*)sqlite3_column_text(stmt, 3);
        backup.created_at = (const char*)sqlite3_column_text(stmt, 4);
        backup.description = (const char*)sqlite3_column_text(stmt, 5);
        backups.push_back(backup);
    }
    
    sqlite3_finalize(stmt);
    return backups;
}

ChatBackup Database::getBackupById(int backupId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    const char* sql = "SELECT id, user_id, backup_name, backup_data, created_at, description FROM chat_backups WHERE id = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return ChatBackup{};
    }
    
    sqlite3_bind_int(stmt, 1, backupId);
    
    ChatBackup backup{};
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        backup.id = sqlite3_column_int(stmt, 0);
        backup.user_id = sqlite3_column_int(stmt, 1);
        backup.backup_name = (const char*)sqlite3_column_text(stmt, 2);
        backup.backup_data = (const char*)sqlite3_column_text(stmt, 3);
        backup.created_at = (const char*)sqlite3_column_text(stmt, 4);
        backup.description = (const char*)sqlite3_column_text(stmt, 5);
    }
    
    sqlite3_finalize(stmt);
    return backup;
}

bool Database::deleteBackup(int backupId, int userId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    const char* sql = "DELETE FROM chat_backups WHERE id = ? AND user_id = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, backupId);
    sqlite3_bind_int(stmt, 2, userId);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

bool Database::restoreFromBackup(int backupId, int userId) {
    // This is a placeholder - actual restoration logic would depend on the backup format
    // For now, we just verify the backup exists and belongs to the user
    ChatBackup backup = getBackupById(backupId);
    if (backup.id == 0 || backup.user_id != userId) {
        return false;
    }
    
    // TODO: Implement actual restoration logic based on backup_data format
    std::cout << "Restoring backup: " << backup.backup_name << std::endl;
    return true;
}

User Database::getUserByEmail(const std::string& email) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    const char* sql = "SELECT id, username, email, password_hash, public_key, created_at, is_online FROM users WHERE email = ?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return User{};
    }
    sqlite3_bind_text(stmt, 1, email.c_str(), -1, SQLITE_STATIC);
    User user{};
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user.id = sqlite3_column_int(stmt, 0);
        user.username = (const char*)sqlite3_column_text(stmt, 1);
        user.email = (const char*)sqlite3_column_text(stmt, 2);
        user.password_hash = (const char*)sqlite3_column_text(stmt, 3);
        user.public_key = (const char*)sqlite3_column_text(stmt, 4);
        user.created_at = (const char*)sqlite3_column_text(stmt, 5);
        user.is_online = sqlite3_column_int(stmt, 6) != 0;
        std::cerr << "[DEBUG] getUserByEmail found: id=" << user.id << ", email='" << user.email << "'" << std::endl;
    } else {
        std::cerr << "[DEBUG] getUserByEmail: no user found for '" << email << "'" << std::endl;
    }
    sqlite3_finalize(stmt);
    return user;
}

// Group invitation methods
bool Database::createGroupInvitation(int groupId, int inviterId, int inviteeId, const std::string& role) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    std::cout << "[DEBUG] createGroupInvitation: groupId=" << groupId << ", inviterId=" << inviterId << ", inviteeId=" << inviteeId << ", role=" << role << std::endl;
    
    // Check if invitation already exists
    const char* checkSql = "SELECT id FROM group_invitations WHERE group_id = ? AND invitee_id = ? AND status = 'pending'";
    sqlite3_stmt* checkStmt;
    
    if (sqlite3_prepare_v2(db_, checkSql, -1, &checkStmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare check statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    sqlite3_bind_int(checkStmt, 1, groupId);
    sqlite3_bind_int(checkStmt, 2, inviteeId);
    
    if (sqlite3_step(checkStmt) == SQLITE_ROW) {
        int existingId = sqlite3_column_int(checkStmt, 0);
        std::cout << "[DEBUG] createGroupInvitation: Found existing invitation with id=" << existingId << ", returning false" << std::endl;
        sqlite3_finalize(checkStmt);
        return false; // Invitation already exists
    }
    
    std::cout << "[DEBUG] createGroupInvitation: No existing invitation found, creating new one" << std::endl;
    sqlite3_finalize(checkStmt);
    
    // Create new invitation
    const char* sql = "INSERT INTO group_invitations (group_id, inviter_id, invitee_id, role, expires_at) VALUES (?, ?, ?, ?, datetime('now', '+24 hours'))";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, groupId);
    sqlite3_bind_int(stmt, 2, inviterId);
    sqlite3_bind_int(stmt, 3, inviteeId);
    sqlite3_bind_text(stmt, 4, role.c_str(), -1, SQLITE_STATIC);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    std::cout << "[DEBUG] createGroupInvitation: Insert result=" << rc << " (SQLITE_DONE=" << SQLITE_DONE << ")" << std::endl;
    
    return rc == SQLITE_DONE;
}

bool Database::acceptGroupInvitation(int invitationId, int inviteeId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    std::cout << "[DEBUG] acceptGroupInvitation: invitationId=" << invitationId << ", inviteeId=" << inviteeId << std::endl;
    
    // Get invitation details (only if not expired)
    const char* getSql = "SELECT group_id, role FROM group_invitations WHERE id = ? AND invitee_id = ? AND status = 'pending' AND expires_at > datetime('now')";
    sqlite3_stmt* getStmt;
    
    if (sqlite3_prepare_v2(db_, getSql, -1, &getStmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare get statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    sqlite3_bind_int(getStmt, 1, invitationId);
    sqlite3_bind_int(getStmt, 2, inviteeId);
    
    int groupId = -1;
    std::string role;
    
    if (sqlite3_step(getStmt) == SQLITE_ROW) {
        groupId = sqlite3_column_int(getStmt, 0);
        role = (const char*)sqlite3_column_text(getStmt, 1);
        std::cout << "[DEBUG] acceptGroupInvitation: Found invitation - groupId=" << groupId << ", role=" << role << std::endl;
    } else {
        std::cout << "[DEBUG] acceptGroupInvitation: No invitation found or already processed/expired" << std::endl;
        std::cout << "[DEBUG] acceptGroupInvitation: SQL error: " << sqlite3_errmsg(db_) << std::endl;
    }
    
    sqlite3_finalize(getStmt);
    
    if (groupId == -1) {
        std::cout << "[DEBUG] acceptGroupInvitation: Returning false - invitation not found" << std::endl;
        return false; // Invitation not found, already processed, or expired
    }
    
    // Update invitation status
    const char* updateSql = "UPDATE group_invitations SET status = 'accepted', responded_at = datetime('now') WHERE id = ?";
    sqlite3_stmt* updateStmt;
    
    if (sqlite3_prepare_v2(db_, updateSql, -1, &updateStmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare update statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    sqlite3_bind_int(updateStmt, 1, invitationId);
    
    int updateRc = sqlite3_step(updateStmt);
    sqlite3_finalize(updateStmt);
    
    std::cout << "[DEBUG] acceptGroupInvitation: Update result=" << updateRc << " (SQLITE_DONE=" << SQLITE_DONE << ")" << std::endl;
    
    if (updateRc != SQLITE_DONE) {
        std::cout << "[DEBUG] acceptGroupInvitation: Update failed" << std::endl;
        return false;
    }
    
    std::cout << "[DEBUG] acceptGroupInvitation: Update successful, adding user to group" << std::endl;
    
    // Add user to group
    std::cout << "[DEBUG] acceptGroupInvitation: About to call addUserToGroup with groupId=" << groupId << ", inviteeId=" << inviteeId << ", role=" << role << std::endl;
    bool addResult = addUserToGroupInternal(groupId, inviteeId, role);
    std::cout << "[DEBUG] acceptGroupInvitation: addUserToGroup result=" << (addResult ? "SUCCESS" : "FAILED") << std::endl;
    
    if (addResult) {
        std::cout << "[DEBUG] acceptGroupInvitation: COMPLETE SUCCESS - User added to group" << std::endl;
    } else {
        std::cout << "[DEBUG] acceptGroupInvitation: FAILED - Could not add user to group" << std::endl;
    }
    
    return addResult;
}

bool Database::declineGroupInvitation(int invitationId, int inviteeId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    const char* sql = "UPDATE group_invitations SET status = 'declined', responded_at = datetime('now') WHERE id = ? AND invitee_id = ? AND status = 'pending'";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, invitationId);
    sqlite3_bind_int(stmt, 2, inviteeId);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

std::vector<GroupInvitation> Database::getPendingInvitations(int userId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    // First, clean up expired invitations
    const char* cleanupSql = "DELETE FROM group_invitations WHERE status = 'pending' AND expires_at <= datetime('now')";
    char* errMsg = nullptr;
    if (sqlite3_exec(db_, cleanupSql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Failed to cleanup expired invitations: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
    
    // Also clean up old processed invitations (but not too frequently)
    static int cleanupCounter = 0;
    if (++cleanupCounter % 10 == 0) { // Clean up every 10th call
        cleanupOldInvitations();
    }
    
    const char* sql = "SELECT id, group_id, inviter_id, invitee_id, role, status, created_at, expires_at, responded_at FROM group_invitations WHERE invitee_id = ? AND status = 'pending' AND expires_at > datetime('now') ORDER BY created_at DESC";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return {};
    }
    
    sqlite3_bind_int(stmt, 1, userId);
    
    std::vector<GroupInvitation> invitations;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        GroupInvitation invitation;
        invitation.id = sqlite3_column_int(stmt, 0);
        invitation.group_id = sqlite3_column_int(stmt, 1);
        invitation.inviter_id = sqlite3_column_int(stmt, 2);
        invitation.invitee_id = sqlite3_column_int(stmt, 3);
        invitation.role = (const char*)sqlite3_column_text(stmt, 4);
        invitation.status = (const char*)sqlite3_column_text(stmt, 5);
        invitation.created_at = (const char*)sqlite3_column_text(stmt, 6);
        invitation.expires_at = (const char*)sqlite3_column_text(stmt, 7);
        invitation.responded_at = sqlite3_column_text(stmt, 8) ? (const char*)sqlite3_column_text(stmt, 8) : "";
        invitations.push_back(invitation);
    }
    
    sqlite3_finalize(stmt);
    return invitations;
}

std::vector<GroupInvitation> Database::getGroupInvitations(int groupId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    const char* sql = "SELECT id, group_id, inviter_id, invitee_id, role, status, created_at, expires_at, responded_at FROM group_invitations WHERE group_id = ? ORDER BY created_at DESC";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return {};
    }
    
    sqlite3_bind_int(stmt, 1, groupId);
    
    std::vector<GroupInvitation> invitations;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        GroupInvitation invitation;
        invitation.id = sqlite3_column_int(stmt, 0);
        invitation.group_id = sqlite3_column_int(stmt, 1);
        invitation.inviter_id = sqlite3_column_int(stmt, 2);
        invitation.invitee_id = sqlite3_column_int(stmt, 3);
        invitation.role = (const char*)sqlite3_column_text(stmt, 4);
        invitation.status = (const char*)sqlite3_column_text(stmt, 5);
        invitation.created_at = (const char*)sqlite3_column_text(stmt, 6);
        invitation.expires_at = (const char*)sqlite3_column_text(stmt, 7);
        invitation.responded_at = sqlite3_column_text(stmt, 8) ? (const char*)sqlite3_column_text(stmt, 8) : "";
        invitations.push_back(invitation);
    }
    
    sqlite3_finalize(stmt);
    return invitations;
}

bool Database::deleteInvitation(int invitationId) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    const char* sql = "DELETE FROM group_invitations WHERE id = ?";
    sqlite3_stmt* stmt;
    
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, invitationId);
    
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

bool Database::cleanupOldInvitations() {
    std::lock_guard<std::mutex> lock(dbMutex_);
    
    // Remove invitations that are older than 7 days and have been processed (accepted/declined)
    const char* sql = "DELETE FROM group_invitations WHERE (status = 'accepted' OR status = 'declined') AND created_at < datetime('now', '-7 days')";
    char* errMsg = nullptr;
    
    if (sqlite3_exec(db_, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Failed to cleanup old invitations: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    
    return true;
} 