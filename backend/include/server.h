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

class WebSocketHandler;
class Database;
class UserManager;
class MessageHandler;

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
    
    // Connection management
    std::map<int, std::thread> clientThreads_;
    std::mutex clientMutex_;
    
    bool setupSocket();
    void acceptConnections();
    void handleClient(int clientSocket);
    void cleanup();
}; 