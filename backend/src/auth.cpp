#include "auth.h"
#include <iostream>
#include <sstream>

Auth::Auth(std::shared_ptr<UserManager> userManager) : userManager_(userManager) {
}

bool Auth::authenticate(const std::string& username, const std::string& password, std::string& token) {
    if (userManager_->authenticateUser(username, password)) {
        User user = userManager_->getUserByUsername(username);
        token = userManager_->generateSessionToken(user.id);
        return !token.empty();
    }
    return false;
}

bool Auth::validateToken(const std::string& token, int& userId) {
    return userManager_->validateSessionToken(token, userId);
}

void Auth::logout(const std::string& token) {
    // TODO: Implement token invalidation
    std::cout << "User logged out: " << token << std::endl;
}

User Auth::getCurrentUser(const std::string& token) {
    int userId;
    if (validateToken(token, userId)) {
        return userManager_->getUserById(userId);
    }
    return User{};
} 