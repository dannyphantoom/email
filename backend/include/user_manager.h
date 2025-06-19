#pragma once

#include <string>
#include <memory>
#include "database.h"

class UserManager {
public:
    UserManager(std::shared_ptr<Database> database);
    
    // Authentication
    bool registerUser(const std::string& username, const std::string& email, 
                     const std::string& password);
    bool authenticateUser(const std::string& username, const std::string& password);
    std::string generateSessionToken(int userId);
    bool validateSessionToken(const std::string& token, int& userId);
    
    // User management
    User getUserByUsername(const std::string& username);
    User getUserById(int id);
    bool updateUserOnlineStatus(int userId, bool online);
    std::vector<User> getAllUsers();
    
    // Validation
    bool isValidUsername(const std::string& username);
    bool isValidEmail(const std::string& email);
    bool isUsernameAvailable(const std::string& username);
    bool isEmailAvailable(const std::string& email);

private:
    std::shared_ptr<Database> database_;
    
    std::string hashPassword(const std::string& password);
    bool verifyPassword(const std::string& password, const std::string& hash);
    std::string generateRandomToken();
}; 