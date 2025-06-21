#pragma once

#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <map>
#include <mutex>
#include <functional>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "account_integration.h"

class WebSocketHandler;
class Database;
class UserManager;
class MessageHandler;
class GroupChat;

class Server {
public:
    Server(int port = 8080);
    ~Server();

    bool initialize();
    void run();
    void stop();
    
    // Getters for components
    std::shared_ptr<Database> getDatabase() const { return database_; }
    std::shared_ptr<UserManager> getUserManager() const { return userManager_; }
    std::shared_ptr<MessageHandler> getMessageHandler() const { return messageHandler_; }
    std::shared_ptr<WebSocketHandler> getWebSocketHandler() const { return wsHandler_; }
    std::shared_ptr<GroupChat> getGroupChat() const { return groupChat_; }

    // HTTP request handling
    void handleRequest(const std::string& request, std::string& response);
    void parseRequest(const std::string& request, std::string& method, std::string& path, std::map<std::string, std::string>& headers, std::string& body);
    
    // Route handlers
    void handleAuthRoutes(const std::string& method, const std::string& path, const std::string& body, std::string& response);
    void handleUserRoutes(const std::string& method, const std::string& path, const std::string& body, std::string& response);
    void handleMessageRoutes(const std::string& method, const std::string& path, const std::string& body, std::string& response);
    void handleChatSessionRoutes(const std::string& method, const std::string& path, const std::string& body, std::string& response);
    void handleGroupRoutes(const std::string& method, const std::string& path, const std::string& body, std::string& response);
    void handleBackupRoutes(const std::string& method, const std::string& path, const std::string& body, std::string& response);
    void handleAccountIntegrationRoutes(const std::string& method, const std::string& path, const std::string& body, std::string& response);
    void handleOAuthCallbackRoutes(const std::string& method, const std::string& path, const std::string& body, std::string& response);
    
    // CORS and utility functions
    void addCORSHeaders(std::string& response);
    std::string getAuthToken(const std::map<std::string, std::string>& headers);
    bool validateToken(const std::string& token, std::string& userId);
    
    // JSON helpers
    std::string createJSONResponse(bool success, const std::string& message, const std::string& data = "", bool fullHttp = false);
    std::string createErrorResponse(const std::string& error, bool fullHttp = false);

private:
    int port_;
    int serverSocket_;
    std::atomic<bool> running_;
    std::thread serverThread_;
    
    // Components
    std::shared_ptr<Database> database_;
    std::shared_ptr<UserManager> userManager_;
    std::shared_ptr<MessageHandler> messageHandler_;
    std::shared_ptr<WebSocketHandler> wsHandler_;
    std::shared_ptr<GroupChat> groupChat_;
    
    // Connection management
    std::map<int, std::thread> clientThreads_;
    std::mutex clientMutex_;
    
    AccountIntegrationManager accountManager;
    
    // Route mapping
    std::map<std::string, std::function<void(const std::string&, const std::string&, const std::string&, std::string&)>> routes;
    
    bool setupSocket();
    void acceptConnections();
    void handleClient(int clientSocket);
    void cleanup();
    void setupRoutes();
}; 