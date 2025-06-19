#pragma once

#include <string>
#include <memory>
#include <functional>
#include "database.h"
#include "user_manager.h"

struct MessageEvent {
    std::string type;
    std::string data;
    int senderId;
    int receiverId;
    int groupId;
};

class MessageHandler {
public:
    MessageHandler(std::shared_ptr<Database> database, 
                  std::shared_ptr<UserManager> userManager);
    
    // Message processing
    bool sendMessage(int senderId, int receiverId, const std::string& content, 
                    const std::string& messageType = "text");
    bool sendGroupMessage(int senderId, int groupId, const std::string& content,
                         const std::string& messageType = "text");
    
    // Message retrieval
    std::vector<Message> getConversation(int userId, int otherUserId, int limit = 50);
    std::vector<Message> getGroupMessages(int groupId, int limit = 50);
    
    // Message status
    bool markMessageAsRead(int messageId);
    bool deleteMessage(int messageId, int userId);
    
    // Event handling
    void setMessageCallback(std::function<void(const MessageEvent&)> callback);
    void handleIncomingMessage(const std::string& messageData);

private:
    std::shared_ptr<Database> database_;
    std::shared_ptr<UserManager> userManager_;
    std::function<void(const MessageEvent&)> messageCallback_;
    
    std::string encryptMessage(const std::string& content);
    std::string decryptMessage(const std::string& encryptedContent);
    bool isUserInGroup(int userId, int groupId);
}; 