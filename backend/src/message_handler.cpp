#include "message_handler.h"
#include <iostream>
#include <sstream>

MessageHandler::MessageHandler(std::shared_ptr<Database> database, 
                             std::shared_ptr<UserManager> userManager)
    : database_(database), userManager_(userManager) {
}

bool MessageHandler::sendMessage(int senderId, int receiverId, const std::string& content, 
                                const std::string& messageType) {
    // Validate users exist
    User sender = userManager_->getUserById(senderId);
    User receiver = userManager_->getUserById(receiverId);
    
    if (sender.id == 0 || receiver.id == 0) {
        std::cerr << "Invalid sender or receiver ID" << std::endl;
        return false;
    }
    
    // Encrypt message content
    std::string encryptedContent = encryptMessage(content);
    
    // Create message
    Message message;
    message.sender_id = senderId;
    message.receiver_id = receiverId;
    message.group_id = 0; // Direct message
    message.content = content;
    message.encrypted_content = encryptedContent;
    message.message_type = messageType;
    
    // Save to database
    if (!database_->saveMessage(message)) {
        std::cerr << "Failed to save message" << std::endl;
        return false;
    }
    
    // Trigger message event
    if (messageCallback_) {
        MessageEvent event;
        event.type = "new_message";
        event.data = content;
        event.senderId = senderId;
        event.receiverId = receiverId;
        event.groupId = 0;
        messageCallback_(event);
    }
    
    return true;
}

bool MessageHandler::sendGroupMessage(int senderId, int groupId, const std::string& content,
                                     const std::string& messageType) {
    // Validate user is in group
    if (!isUserInGroup(senderId, groupId)) {
        std::cerr << "User is not a member of the group" << std::endl;
        return false;
    }
    
    // Encrypt message content
    std::string encryptedContent = encryptMessage(content);
    
    // Create message
    Message message;
    message.sender_id = senderId;
    message.receiver_id = 0; // Group message
    message.group_id = groupId;
    message.content = content;
    message.encrypted_content = encryptedContent;
    message.message_type = messageType;
    
    // Save to database
    if (!database_->saveMessage(message)) {
        std::cerr << "Failed to save group message" << std::endl;
        return false;
    }
    
    // Trigger message event
    if (messageCallback_) {
        MessageEvent event;
        event.type = "new_group_message";
        event.data = content;
        event.senderId = senderId;
        event.receiverId = 0;
        event.groupId = groupId;
        messageCallback_(event);
    }
    
    return true;
}

std::vector<Message> MessageHandler::getConversation(int userId, int otherUserId, int limit) {
    return database_->getMessages(userId, otherUserId, limit);
}

std::vector<Message> MessageHandler::getGroupMessages(int groupId, int limit) {
    return database_->getGroupMessages(groupId, limit);
}

bool MessageHandler::markMessageAsRead(int messageId) {
    return database_->markMessageAsRead(messageId);
}

bool MessageHandler::deleteMessage(int messageId, int userId) {
    // TODO: Implement message deletion with permission check
    (void)userId; // Suppress unused parameter warning
    return database_->deleteMessage(messageId);
}

void MessageHandler::setMessageCallback(std::function<void(const MessageEvent&)> callback) {
    messageCallback_ = callback;
}

void MessageHandler::handleIncomingMessage(const std::string& messageData) {
    // TODO: Parse JSON message and route appropriately
    std::cout << "Received message: " << messageData << std::endl;
}

std::string MessageHandler::encryptMessage(const std::string& content) {
    // TODO: Implement actual encryption
    // For now, just return the content as-is
    return content;
}

std::string MessageHandler::decryptMessage(const std::string& encryptedContent) {
    // TODO: Implement actual decryption
    // For now, just return the content as-is
    return encryptedContent;
}

bool MessageHandler::isUserInGroup(int userId, int groupId) {
    return database_->isGroupMember(groupId, userId);
} 