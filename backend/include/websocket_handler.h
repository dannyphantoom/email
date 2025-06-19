#pragma once

#include <string>
#include <map>
#include <set>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

class MessageHandler;
class UserManager;

struct WebSocketFrame {
    bool fin;
    uint8_t opcode;
    bool masked;
    uint64_t payload_length;
    std::string payload;
    std::string masking_key;
};

struct WebSocketConnection {
    int socket;
    std::string remote_address;
    int user_id;
    bool authenticated;
    std::string username;
    std::thread read_thread;
    std::atomic<bool> active;
    
    WebSocketConnection(int sock, const std::string& addr) 
        : socket(sock), remote_address(addr), user_id(-1), 
          authenticated(false), active(true) {}
};

class WebSocketHandler {
public:
    WebSocketHandler(std::shared_ptr<MessageHandler> msgHandler, 
                    std::shared_ptr<UserManager> userManager);
    ~WebSocketHandler();

    void handleConnection(int clientSocket, const std::string& remoteAddress);
    void broadcastMessage(const std::string& message, const std::set<int>& userIds);
    void sendToUser(int userId, const std::string& message);
    void disconnectUser(int userId);
    
    // Connection management
    void addConnection(int userId, std::shared_ptr<WebSocketConnection> conn);
    void removeConnection(int userId);
    std::shared_ptr<WebSocketConnection> getConnection(int userId);
    
    // WebSocket protocol
    bool performHandshake(int socket, const std::string& request);
    std::string createHandshakeResponse(const std::string& key);
    WebSocketFrame parseFrame(const std::string& data);
    std::string createFrame(const std::string& payload, uint8_t opcode = 0x01);
    std::string maskData(const std::string& data, const std::string& key);

private:
    std::shared_ptr<MessageHandler> messageHandler_;
    std::shared_ptr<UserManager> userManager_;
    
    // Connection tracking
    std::map<int, std::shared_ptr<WebSocketConnection>> connections_;
    std::mutex connectionsMutex_;
    
    // WebSocket constants
    static const uint8_t OPCODE_CONTINUATION = 0x0;
    static const uint8_t OPCODE_TEXT = 0x1;
    static const uint8_t OPCODE_BINARY = 0x2;
    static const uint8_t OPCODE_CLOSE = 0x8;
    static const uint8_t OPCODE_PING = 0x9;
    static const uint8_t OPCODE_PONG = 0xA;
    
    void handleClient(std::shared_ptr<WebSocketConnection> conn);
    void processMessage(std::shared_ptr<WebSocketConnection> conn, const std::string& message);
    void sendFrame(std::shared_ptr<WebSocketConnection> conn, const std::string& payload, uint8_t opcode);
    void closeConnection(std::shared_ptr<WebSocketConnection> conn, uint16_t code = 1000);
    
    // Utility functions
    std::string generateWebSocketKey();
    std::string sha1Base64(const std::string& input);
    std::string base64Encode(const std::string& data);
    uint16_t htons(uint16_t hostshort);
    
    // HTTP API handlers
    void handleRegister(std::shared_ptr<WebSocketConnection> conn, const std::string& request);
    void handleLogin(std::shared_ptr<WebSocketConnection> conn, const std::string& request);
    void handleCorsPreflight(std::shared_ptr<WebSocketConnection> conn);
    void sendErrorResponse(std::shared_ptr<WebSocketConnection> conn, int statusCode, const std::string& message);
}; 