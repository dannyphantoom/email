#pragma once

#include <string>
#include <memory>
#include "user_manager.h"

class Auth {
public:
    Auth(std::shared_ptr<UserManager> userManager);
    
    // Authentication
    bool authenticate(const std::string& username, const std::string& password, std::string& token);
    bool validateToken(const std::string& token, int& userId);
    void logout(const std::string& token);
    
    // User info
    User getCurrentUser(const std::string& token);

private:
    std::shared_ptr<UserManager> userManager_;
}; 