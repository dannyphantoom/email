#include "user_manager.h"
#include <iostream>
#include <regex>
#include <random>
#include <sstream>
#include <iomanip>
#include <openssl/evp.h>

UserManager::UserManager(std::shared_ptr<Database> database) : database_(database) {
}

bool UserManager::registerUser(const std::string& username, const std::string& email, 
                              const std::string& password) {
    // Validate input
    if (!isValidUsername(username)) {
        std::cerr << "Invalid username format" << std::endl;
        return false;
    }
    
    if (!isValidEmail(email)) {
        std::cerr << "Invalid email format" << std::endl;
        return false;
    }
    
    if (password.length() < 6) {
        std::cerr << "Password must be at least 6 characters" << std::endl;
        return false;
    }
    
    // Check availability
    if (!isUsernameAvailable(username)) {
        std::cerr << "Username already taken" << std::endl;
        return false;
    }
    
    if (!isEmailAvailable(email)) {
        std::cerr << "Email already registered" << std::endl;
        return false;
    }
    
    // Hash password
    std::string passwordHash = hashPassword(password);
    
    // Generate public key (placeholder)
    std::string publicKey = "public_key_" + username;
    
    // Create user
    return database_->createUser(username, email, passwordHash, publicKey);
}

bool UserManager::authenticateUser(const std::string& username, const std::string& password) {
    User user = database_->getUserByUsername(username);
    if (user.id == 0) {
        return false; // User not found
    }
    
    return verifyPassword(password, user.password_hash);
}

std::string UserManager::generateSessionToken(int userId) {
    std::string token = generateRandomToken();
    
    // Set expiration to 24 hours from now
    time_t now = time(nullptr);
    time_t expires = now + (24 * 60 * 60); // 24 hours
    
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&expires), "%Y-%m-%d %H:%M:%S");
    std::string expiresAt = ss.str();
    
    if (database_->saveSession(token, userId, expiresAt)) {
        return token;
    }
    
    return "";
}

bool UserManager::validateSessionToken(const std::string& token, int& userId) {
    userId = database_->getUserIdFromSession(token);
    return userId != -1;
}

User UserManager::getUserByUsername(const std::string& username) {
    return database_->getUserByUsername(username);
}

User UserManager::getUserById(int id) {
    return database_->getUserById(id);
}

bool UserManager::updateUserOnlineStatus(int userId, bool online) {
    return database_->updateUserOnlineStatus(userId, online);
}

std::vector<User> UserManager::getAllUsers() {
    return database_->getAllUsers();
}

bool UserManager::isValidUsername(const std::string& username) {
    // Username must be 3-20 characters, alphanumeric and underscores only
    std::regex usernameRegex("^[a-zA-Z0-9_]{3,20}$");
    return std::regex_match(username, usernameRegex);
}

bool UserManager::isValidEmail(const std::string& email) {
    // Basic email validation
    std::regex emailRegex("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
    return std::regex_match(email, emailRegex);
}

bool UserManager::isUsernameAvailable(const std::string& username) {
    User user = database_->getUserByUsername(username);
    return user.id == 0; // User not found means username is available
}

bool UserManager::isEmailAvailable(const std::string& email) {
    // TODO: Implement email availability check
    // For now, assume all emails are available
    (void)email; // Suppress unused parameter warning
    return true;
}

std::string UserManager::hashPassword(const std::string& password) {
    // Use EVP interface for OpenSSL 3.0 compatibility
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        return "";
    }
    
    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        return "";
    }
    
    if (EVP_DigestUpdate(ctx, password.c_str(), password.length()) != 1) {
        EVP_MD_CTX_free(ctx);
        return "";
    }
    
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen;
    if (EVP_DigestFinal_ex(ctx, hash, &hashLen) != 1) {
        EVP_MD_CTX_free(ctx);
        return "";
    }
    
    EVP_MD_CTX_free(ctx);
    
    std::stringstream ss;
    for (unsigned int i = 0; i < hashLen; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

bool UserManager::verifyPassword(const std::string& password, const std::string& hash) {
    std::string passwordHash = hashPassword(password);
    return passwordHash == hash;
}

std::string UserManager::generateRandomToken() {
    const std::string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> distribution(0, chars.size() - 1);
    
    std::string token;
    for (int i = 0; i < 32; ++i) {
        token += chars[distribution(generator)];
    }
    
    return token;
} 