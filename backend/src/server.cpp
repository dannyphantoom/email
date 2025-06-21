#include "server.h"
#include "database.h"
#include "user_manager.h"
#include "message_handler.h"
#include "group_chat.h"
#include "auth.h"
#include "websocket_handler.h"
#include <iostream>
#include <sstream>
#include <regex>
#include <nlohmann/json.hpp>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <chrono>
#include <fcntl.h>
#include <algorithm>

using json = nlohmann::json;

Server::Server(int port, const std::string& dbPath) : port_(port), running_(false), database_(std::make_shared<Database>(dbPath)), 
                          userManager_(std::make_shared<UserManager>(database_)),
                          messageHandler_(std::make_shared<MessageHandler>(database_, userManager_)),
                          groupChat_(std::make_shared<GroupChat>(database_)),
                          wsHandler_(std::make_shared<WebSocketHandler>(messageHandler_, userManager_, this)),
                          serverSocket_(-1) {
    setupRoutes();
}

Server::~Server() {
    stop();
}

void Server::setupRoutes() {
    routes["/auth/register"] = [this](const std::string& method, const std::string& path, const std::string& body, const std::map<std::string, std::string>& headers, std::string& response) {
        handleAuthRoutes(method, path, body, response);
    };
    routes["/auth/login"] = [this](const std::string& method, const std::string& path, const std::string& body, const std::map<std::string, std::string>& headers, std::string& response) {
        handleAuthRoutes(method, path, body, response);
    };
    routes["/auth/logout"] = [this](const std::string& method, const std::string& path, const std::string& body, const std::map<std::string, std::string>& headers, std::string& response) {
        handleAuthRoutes(method, path, body, response);
    };
    routes["/users"] = [this](const std::string& method, const std::string& path, const std::string& body, const std::map<std::string, std::string>& headers, std::string& response) {
        handleUserRoutes(method, path, body, headers, response);
    };
    routes["/api/users"] = [this](const std::string& method, const std::string& path, const std::string& body, const std::map<std::string, std::string>& headers, std::string& response) {
        handleUserRoutes(method, path, body, headers, response);
    };
    routes["/messages"] = [this](const std::string& method, const std::string& path, const std::string& body, const std::map<std::string, std::string>& headers, std::string& response) {
        handleMessageRoutes(method, path, body, headers, response);
    };
    routes["/groups"] = [this](const std::string& method, const std::string& path, const std::string& body, const std::map<std::string, std::string>& headers, std::string& response) {
        handleGroupRoutes(method, path, body, headers, response);
    };
    
    routes["/api/groups"] = [this](const std::string& method, const std::string& path, const std::string& body, const std::map<std::string, std::string>& headers, std::string& response) {
        handleGroupRoutes(method, path, body, headers, response);
    };
    
    // Chat sessions routes
    routes["/chat-sessions"] = [this](const std::string& method, const std::string& path, const std::string& body, const std::map<std::string, std::string>& headers, std::string& response) {
        handleChatSessionRoutes(method, path, body, headers, response);
    };
    routes["/api/chat-sessions"] = [this](const std::string& method, const std::string& path, const std::string& body, const std::map<std::string, std::string>& headers, std::string& response) {
        handleChatSessionRoutes(method, path, body, headers, response);
    };
    
    // Backup routes
    routes["/backup"] = [this](const std::string& method, const std::string& path, const std::string& body, const std::map<std::string, std::string>& headers, std::string& response) {
        handleBackupRoutes(method, path, body, headers, response);
    };
    
    // Account integration routes
    routes["/integration/accounts"] = [this](const std::string& method, const std::string& path, const std::string& body, const std::map<std::string, std::string>& headers, std::string& response) {
        handleAccountIntegrationRoutes(method, path, body, headers, response);
    };
    routes["/integration/connect/gmail"] = [this](const std::string& method, const std::string& path, const std::string& body, const std::map<std::string, std::string>& headers, std::string& response) {
        handleAccountIntegrationRoutes(method, path, body, headers, response);
    };
    routes["/integration/connect/gmail/oauth2"] = [this](const std::string& method, const std::string& path, const std::string& body, const std::map<std::string, std::string>& headers, std::string& response) {
        handleAccountIntegrationRoutes(method, path, body, headers, response);
    };
    routes["/integration/gmail/oauth2/url"] = [this](const std::string& method, const std::string& path, const std::string& body, const std::map<std::string, std::string>& headers, std::string& response) {
        handleAccountIntegrationRoutes(method, path, body, headers, response);
    };
    routes["/oauth/gmail/callback"] = [this](const std::string& method, const std::string& path, const std::string& body, const std::map<std::string, std::string>& headers, std::string& response) {
        handleOAuthCallbackRoutes(method, path, body, headers, response);
    };
    routes["/integration/connect/whatsapp"] = [this](const std::string& method, const std::string& path, const std::string& body, const std::map<std::string, std::string>& headers, std::string& response) {
        handleAccountIntegrationRoutes(method, path, body, headers, response);
    };
    routes["/integration/messages"] = [this](const std::string& method, const std::string& path, const std::string& body, const std::map<std::string, std::string>& headers, std::string& response) {
        handleAccountIntegrationRoutes(method, path, body, headers, response);
    };
    routes["/integration/sync"] = [this](const std::string& method, const std::string& path, const std::string& body, const std::map<std::string, std::string>& headers, std::string& response) {
        handleAccountIntegrationRoutes(method, path, body, headers, response);
    };
    
    // Invitations routes
    routes["/invitations"] = [this](const std::string& method, const std::string& path, const std::string& body, const std::map<std::string, std::string>& headers, std::string& response) {
        handleGroupRoutes(method, path, body, headers, response);
    };
    routes["/api/invitations"] = [this](const std::string& method, const std::string& path, const std::string& body, const std::map<std::string, std::string>& headers, std::string& response) {
        handleGroupRoutes(method, path, body, headers, response);
    };
}

bool Server::initialize() {
    std::cout << "Initializing Cockpit Messenger Server..." << std::endl;
    
    // Initialize database
    if (!database_->initialize()) {
        std::cerr << "Failed to initialize database" << std::endl;
        return false;
    }
    
    // Setup server socket
    if (!setupSocket()) {
        std::cerr << "Failed to setup server socket" << std::endl;
        return false;
    }
    
    std::cout << "Server initialized successfully on port " << port_ << std::endl;
    return true;
}

bool Server::setupSocket() {
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return false;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options" << std::endl;
        close(serverSocket_);
        return false;
    }
    
    // Set non-blocking
    int flags = fcntl(serverSocket_, F_GETFL, 0);
    fcntl(serverSocket_, F_SETFL, flags | O_NONBLOCK);
    
    // Bind socket
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port_);
    
    if (bind(serverSocket_, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Failed to bind socket" << std::endl;
        close(serverSocket_);
        return false;
    }
    
    // Listen for connections
    if (listen(serverSocket_, SOMAXCONN) < 0) {
        std::cerr << "Failed to listen on socket" << std::endl;
        close(serverSocket_);
        return false;
    }
    
    return true;
}

void Server::run() {
    running_ = true;
    std::cout << "Server is running. Press Ctrl+C to stop." << std::endl;
    
    // Simple blocking accept loop instead of epoll for now
    while (running_) {
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        
        int clientSocket = accept(serverSocket_, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No connection available, sleep briefly to prevent 100% CPU usage
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            if (errno == EINTR) {
                // Interrupted by signal, continue
                continue;
            }
            std::cerr << "Failed to accept connection: " << strerror(errno) << std::endl;
            continue;
        }
        
        // Get client address
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
        std::string remoteAddress = std::string(clientIP) + ":" + std::to_string(ntohs(clientAddr.sin_port));
        
        std::cout << "New connection from " << remoteAddress << std::endl;
        
        try {
            // Handle WebSocket upgrade or HTTP request in a separate thread
            std::thread clientThread([this, clientSocket, remoteAddress]() {
                wsHandler_->handleConnection(clientSocket, remoteAddress);
            });
            clientThread.detach(); // Let the thread run independently
        } catch (const std::exception& e) {
            std::cerr << "Exception handling connection: " << e.what() << std::endl;
            close(clientSocket);
        } catch (...) {
            std::cerr << "Unknown exception handling connection" << std::endl;
            close(clientSocket);
        }
    }
    
    cleanup();
}

void Server::stop() {
    running_ = false;
    std::cout << "Stopping server..." << std::endl;
}

void Server::cleanup() {
    if (serverSocket_ != -1) {
        close(serverSocket_);
        serverSocket_ = -1;
    }
}

// JSON helper methods
std::string Server::createJSONResponse(bool success, const std::string& message, const std::string& data, bool fullHttp) {
    json response;
    response["success"] = success;
    response["message"] = message;
    if (!data.empty()) {
        response["data"] = json::parse(data);
    }
    std::string jsonStr = response.dump();
    if (fullHttp) {
        return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n" + jsonStr;
    } else {
        return jsonStr;
    }
}

std::string Server::createErrorResponse(const std::string& error, bool fullHttp) {
    std::string jsonStr = createJSONResponse(false, error, "", false);
    if (fullHttp) {
        return "HTTP/1.1 400 Bad Request\r\nContent-Type: application/json\r\n\r\n" + jsonStr;
    } else {
        return jsonStr;
    }
}

// CORS headers
void Server::addCORSHeaders(std::string& response) {
    std::string corsHeaders = "Access-Control-Allow-Origin: *\r\n";
    corsHeaders += "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n";
    corsHeaders += "Access-Control-Allow-Headers: Content-Type, Authorization\r\n";
    corsHeaders += "Access-Control-Max-Age: 86400\r\n";
    
    // Insert CORS headers after the status line
    size_t pos = response.find("\r\n");
    if (pos != std::string::npos) {
        response.insert(pos + 2, corsHeaders);
    }
}

// Authentication token handling
std::string Server::getAuthToken(const std::map<std::string, std::string>& headers) {
    auto it = headers.find("authorization");
    if (it != headers.end()) {
        std::string authHeader = it->second;
        if (authHeader.substr(0, 7) == "Bearer ") {
            return authHeader.substr(7);
        }
    }
    return "";
}

void Server::handleAccountIntegrationRoutes(const std::string& method, const std::string& path, const std::string& body, const std::map<std::string, std::string>& headers, std::string& response) {
    try {
        // Extract user ID from Authorization header
        int userIdInt = 0; // Default for development
        auto authIt = headers.find("Authorization");
        if (authIt != headers.end()) {
            std::string authHeader = authIt->second;
            if (authHeader.find("Bearer ") == 0) {
                std::string token = authHeader.substr(7);
                if (userManager_->validateSessionToken(token, userIdInt)) {
                    // userIdInt is already set by validateSessionToken
                } else {
                    response = createErrorResponse("Invalid token", true);
                    return;
                }
            }
        }
        
        std::string userId = std::to_string(userIdInt);
        
        if (method == "GET" && path == "/integration/accounts") {
            // Get user's connected accounts
            auto accounts = accountManager.getUserAccounts(userId);
            json accountsArray = json::array();
            
            for (const auto& account : accounts) {
                json accountJson;
                accountJson["id"] = account.id;
                accountJson["type"] = static_cast<int>(account.type);
                accountJson["provider"] = static_cast<int>(account.provider);
                accountJson["email"] = account.email;
                accountJson["username"] = account.username;
                accountJson["isActive"] = account.isActive;
                accountJson["lastSync"] = std::chrono::duration_cast<std::chrono::seconds>(
                    account.lastSync.time_since_epoch()).count();
                accountJson["createdAt"] = std::chrono::duration_cast<std::chrono::seconds>(
                    account.createdAt.time_since_epoch()).count();
                
                accountsArray.push_back(accountJson);
            }
            
            response = createJSONResponse(true, "Accounts retrieved successfully", accountsArray.dump(), true);
        }
        else if (method == "GET" && path == "/integration/messages") {
            // Get unified messages from all connected accounts
            auto messages = accountManager.fetchNewMessages(userId);
            json messagesArray = json::array();
            
            for (const auto& message : messages) {
                json messageJson;
                messageJson["id"] = message.id;
                messageJson["accountId"] = message.accountId;
                messageJson["sender"] = message.sender;
                messageJson["recipient"] = message.recipient;
                messageJson["subject"] = message.subject;
                messageJson["content"] = message.content;
                messageJson["messageType"] = message.messageType;
                messageJson["timestamp"] = std::chrono::duration_cast<std::chrono::seconds>(
                    message.timestamp.time_since_epoch()).count();
                messageJson["isRead"] = message.isRead;
                messageJson["isImportant"] = message.isImportant;
                messageJson["attachments"] = json::array();
                for (const auto& attachment : message.attachments) {
                    messageJson["attachments"].push_back(attachment);
                }
                messageJson["metadata"] = json::object();
                for (const auto& [key, value] : message.metadata) {
                    messageJson["metadata"][key] = value;
                }
                
                messagesArray.push_back(messageJson);
            }
            
            response = createJSONResponse(true, "Messages retrieved successfully", messagesArray.dump(), true);
        }
        else if (method == "POST" && path == "/integration/connect/gmail") {
            // Connect Gmail account
            json requestJson = json::parse(body);
            std::string email = requestJson["email"];
            std::string password = requestJson["password"];
            
            bool success = accountManager.connectGmail(userId, email, password);
            
            if (success) {
                response = createJSONResponse(true, "Gmail account connected successfully", "", true);
            } else {
                response = createErrorResponse("Failed to connect Gmail account", true);
            }
        }
        else if (method == "GET" && path == "/integration/gmail/oauth2/url") {
            // Return Gmail OAuth2 authorization URL
            std::string url = accountManager.getGmailOAuth2Url();
            json data;
            data["url"] = url;
            response = createJSONResponse(true, "Gmail OAuth2 URL generated", data.dump(), true);
        }
        else if (method == "POST" && path == "/integration/connect/gmail/oauth2") {
            // Complete Gmail OAuth2 connection using authorization code
            json requestJson = json::parse(body);
            std::string email = requestJson["email"];
            std::string code = requestJson["code"];

            std::string accessToken;
            std::string refreshToken;
            if (!accountManager.exchangeGmailCodeForTokens(code, accessToken, refreshToken)) {
                response = createErrorResponse("Failed to exchange OAuth2 code", true);
                return;
            }

            bool success = accountManager.connectGmailOAuth2(userId, email, accessToken, refreshToken);
            if (success) {
                response = createJSONResponse(true, "Gmail account connected via OAuth2", "", true);
            } else {
                response = createErrorResponse("Failed to connect Gmail via OAuth2", true);
            }
        }
        else if (method == "POST" && path == "/integration/connect/whatsapp") {
            // Connect WhatsApp account
            json requestJson = json::parse(body);
            std::string phoneNumber = requestJson["phoneNumber"];
            std::string password = requestJson["password"];
            
            bool success = accountManager.connectWhatsApp(userId, phoneNumber, password);
            
            if (success) {
                response = createJSONResponse(true, "WhatsApp account connected successfully", "", true);
            } else {
                response = createErrorResponse("Failed to connect WhatsApp account", true);
            }
        }
        else if (method == "POST" && path == "/integration/connect/telegram") {
            // Connect Telegram account
            json requestJson = json::parse(body);
            std::string phoneNumber = requestJson["phoneNumber"];
            std::string code = requestJson["code"];
            
            bool success = accountManager.connectTelegram(userId, phoneNumber, code);
            
            if (success) {
                response = createJSONResponse(true, "Telegram account connected successfully", "", true);
            } else {
                response = createErrorResponse("Failed to connect Telegram account", true);
            }
        }
        else if (method == "POST" && path == "/integration/sync") {
            // Manual sync trigger
            json requestJson = json::parse(body);
            std::string accountId = requestJson["accountId"];
            
            bool success = accountManager.syncAccount(userId, accountId);
            
            if (success) {
                response = createJSONResponse(true, "Account synced successfully", "", true);
            } else {
                response = createErrorResponse("Failed to sync account", true);
            }
        }
        else {
            response = createErrorResponse("Method not allowed", true);
        }
    } catch (const std::exception& e) {
        response = createErrorResponse("Internal server error: " + std::string(e.what()), true);
    }
}

void Server::handleAuthRoutes(const std::string& method, const std::string& path, const std::string& body, std::string& response) {
    response = createErrorResponse("Not implemented", true);
}

void Server::handleUserRoutes(const std::string& method, const std::string& path, const std::string& body, const std::map<std::string, std::string>& headers, std::string& response) {
    std::cout << "[DEBUG] handleUserRoutes called with:" << std::endl;
    std::cout << "[DEBUG] Method: " << method << std::endl;
    std::cout << "[DEBUG] Path: " << path << std::endl;
    std::cout << "[DEBUG] Body: " << body << std::endl;
    
    // Debug headers
    std::cout << "[DEBUG] Headers received:" << std::endl;
    for (const auto& header : headers) {
        std::cout << "[DEBUG] Header: " << header.first << " = " << header.second << std::endl;
    }
    
    try {
        // Extract user ID from Authorization header
        int userIdInt = 0; // Initialize to 0
        std::string authHeader;
        for (const auto& header : headers) {
            std::string headerName = header.first;
            std::transform(headerName.begin(), headerName.end(), headerName.begin(), ::tolower);
            if (headerName == "authorization") {
                authHeader = header.second;
                break;
            }
        }
        
        if (!authHeader.empty()) {
            std::cout << "[DEBUG] Authorization header found: " << authHeader << std::endl;
            if (authHeader.find("Bearer ") == 0) {
                std::string token = authHeader.substr(7);
                std::cout << "[DEBUG] Token extracted: " << token << std::endl;
                if (!userManager_->validateSessionToken(token, userIdInt)) {
                    std::cout << "[DEBUG] Token validation failed" << std::endl;
                    response = createErrorResponse("Unauthorized", true);
                    return;
                }
            }
        } else {
            std::cout << "[DEBUG] No Authorization header found" << std::endl;
        }
        
        if (method == "GET" && (path == "/users" || path == "/api/users")) {
            std::cout << "[DEBUG] Getting all users" << std::endl;
            // Get all users (for search functionality)
            auto users = userManager_->getAllUsers();
            json usersArray = json::array();
            
            for (const auto& user : users) {
                // Don't include the current user in the results
                if (user.id != userIdInt) {
                    json userJson;
                    userJson["id"] = user.id;
                    userJson["username"] = user.username;
                    userJson["email"] = user.email;
                    userJson["is_online"] = user.is_online;
                    userJson["created_at"] = user.created_at;
                    
                    usersArray.push_back(userJson);
                }
            }
            
            response = createJSONResponse(true, "Users retrieved successfully", usersArray.dump(), true);
        }
        else if (method == "GET" && (path.find("/users/search/") == 0 || path.find("/api/users/search/") == 0)) {
            std::cout << "[DEBUG] Searching users" << std::endl;
            // Search users by username
            size_t searchStart = path.find("/users/search/") != std::string::npos ? path.find("/users/search/") + 13 : path.find("/api/users/search/") + 17;
            std::string searchTerm = path.substr(searchStart);
            
            // Remove leading slash if present
            if (!searchTerm.empty() && searchTerm[0] == '/') {
                searchTerm = searchTerm.substr(1);
            }
            
            std::cout << "[DEBUG] Search term: '" << searchTerm << "'" << std::endl;
            
            auto users = userManager_->getAllUsers();
            std::cout << "[DEBUG] Found " << users.size() << " total users" << std::endl;
            json usersArray = json::array();
            
            // Convert search term to lowercase for case-insensitive search
            std::string searchTermLower = searchTerm;
            std::transform(searchTermLower.begin(), searchTermLower.end(), searchTermLower.begin(), ::tolower);
            
            for (const auto& user : users) {
                std::cout << "[DEBUG] Checking user: " << user.username << " (ID: " << user.id << ")" << std::endl;
                // Don't include the current user in the results
                if (user.id != userIdInt) {
                    // Convert username to lowercase for case-insensitive comparison
                    std::string usernameLower = user.username;
                    std::transform(usernameLower.begin(), usernameLower.end(), usernameLower.begin(), ::tolower);
                    
                    if (usernameLower.find(searchTermLower) != std::string::npos) {
                        std::cout << "[DEBUG] User " << user.username << " matches search term" << std::endl;
                        json userJson;
                        userJson["id"] = user.id;
                        userJson["username"] = user.username;
                        userJson["email"] = user.email;
                        userJson["is_online"] = user.is_online;
                        userJson["created_at"] = user.created_at;
                        
                        usersArray.push_back(userJson);
                    }
                }
            }
            
            std::cout << "[DEBUG] Returning " << usersArray.size() << " matching users" << std::endl;
            response = createJSONResponse(true, "Users found successfully", usersArray.dump(), true);
        }
        else {
            std::cout << "[DEBUG] Method not allowed: " << method << " " << path << std::endl;
            response = createErrorResponse("Method not allowed", true);
        }
    } catch (const std::exception& e) {
        std::cout << "[DEBUG] Exception in handleUserRoutes: " << e.what() << std::endl;
        response = createErrorResponse("Internal server error: " + std::string(e.what()), true);
    }
}

void Server::handleMessageRoutes(const std::string& method, const std::string& path, const std::string& body, const std::map<std::string, std::string>& headers, std::string& response) {
    try {
        // Extract user ID from Authorization header
        int userIdInt = 0; // Default for development
        auto authIt = headers.find("Authorization");
        if (authIt != headers.end()) {
            std::string authHeader = authIt->second;
            if (authHeader.find("Bearer ") == 0) {
                std::string token = authHeader.substr(7);
                if (userManager_->validateSessionToken(token, userIdInt)) {
                    // userIdInt is already set by validateSessionToken
                } else {
                    response = createErrorResponse("Unauthorized", true);
                    return;
                }
            }
        }
        
        if (method == "GET" && path.find("/messages/") == 0) {
            // Get messages for a chat session
            size_t sessionIdStart = path.find("/messages/") + 10;
            int sessionId = std::stoi(path.substr(sessionIdStart));
            
            auto messages = database_->getMessages(userIdInt, sessionId, 50);
            json messagesArray = json::array();
            
            for (const auto& msg : messages) {
                json messageJson;
                messageJson["id"] = msg.id;
                messageJson["content"] = msg.content;
                messageJson["sender_id"] = msg.sender_id;
                messageJson["timestamp"] = msg.timestamp;
                messageJson["is_read"] = msg.is_read;
                messageJson["message_type"] = msg.message_type;
                
                messagesArray.push_back(messageJson);
            }
            
            response = createJSONResponse(true, "Messages retrieved successfully", messagesArray.dump(), true);
        }
        else if (method == "POST" && path.find("/messages/") == 0) {
            // Send a message
            size_t sessionIdStart = path.find("/messages/") + 10;
            int sessionId = std::stoi(path.substr(sessionIdStart));
            
            json requestJson = json::parse(body);
            std::string content = requestJson["content"];
            std::string messageType = requestJson.value("type", "text");
            
            Message message;
            message.sender_id = userIdInt;
            message.receiver_id = sessionId;
            message.content = content;
            message.message_type = messageType;
            
            bool success = database_->saveMessage(message);
            
            if (success) {
                response = createJSONResponse(true, "Message sent successfully", "", true);
            } else {
                response = createErrorResponse("Failed to send message", true);
            }
        }
        else {
            response = createErrorResponse("Method not allowed", true);
        }
    } catch (const std::exception& e) {
        response = createErrorResponse("Internal server error: " + std::string(e.what()), true);
    }
}

void Server::handleGroupRoutes(const std::string& method, const std::string& path, const std::string& body, const std::map<std::string, std::string>& headers, std::string& response) {
    std::cout << "[DEBUG] handleGroupRoutes called with:" << std::endl;
    std::cout << "[DEBUG] Method: " << method << std::endl;
    std::cout << "[DEBUG] Path: " << path << std::endl;
    std::cout << "[DEBUG] Body: " << body << std::endl;
    
    // Extract user ID from Authorization header
    int userIdInt = 0;
    std::cout << "[DEBUG] Initial userIdInt: " << userIdInt << std::endl;
    
    // Accept both 'Authorization' and 'authorization' headers (case-insensitive)
    auto authIt = headers.find("Authorization");
    if (authIt == headers.end()) {
        authIt = headers.find("authorization");
    }
    if (authIt != headers.end()) {
        std::string authHeader = authIt->second;
        std::cout << "[DEBUG] Authorization header: " << authHeader << std::endl;
        if (authHeader.find("Bearer ") == 0) {
            std::string token = authHeader.substr(7);
            std::cout << "[DEBUG] Extracted token: " << token << std::endl;
            if (userManager_->validateSessionToken(token, userIdInt)) {
                std::cout << "[DEBUG] Token validation SUCCESS for user: " << userIdInt << std::endl;
                std::cout << "[DEBUG] userIdInt after token validation: " << userIdInt << std::endl;
            } else {
                std::cout << "[DEBUG] Token validation FAILED for token: " << token << std::endl;
                response = createErrorResponse("Invalid token", true);
                return;
            }
        } else {
            std::cout << "[DEBUG] Authorization header does not start with 'Bearer '" << std::endl;
            response = createErrorResponse("Invalid authorization header format", true);
            return;
        }
    } else {
        std::cout << "[DEBUG] Authorization header not found" << std::endl;
        response = createErrorResponse("Authorization header required", true);
        return;
    }
    
    std::cout << "[DEBUG] Final User ID from token: " << userIdInt << std::endl;
    
    try {
        if (method == "POST" && (path == "/groups" || path == "/api/groups")) {
            // Create new group
            std::cout << "[DEBUG] Creating group for user " << userIdInt << std::endl;
            std::cout << "[DEBUG] Request body: " << body << std::endl;
            
            try {
                json requestJson = json::parse(body);
                std::string name = requestJson["name"];
                std::string description = requestJson["description"];
                
                std::cout << "[DEBUG] Group name: " << name << std::endl;
                std::cout << "[DEBUG] Group description: " << description << std::endl;
                
                int groupId = database_->createGroup(name, description, userIdInt);
                std::cout << "[DEBUG] Created group ID: " << groupId << std::endl;
                
                if (groupId > 0) {
                    // Add creator as admin member
                    bool memberAdded = database_->addUserToGroup(groupId, userIdInt, "admin");
                    std::cout << "[DEBUG] Added creator to group: " << (memberAdded ? "SUCCESS" : "FAILED") << std::endl;
                    
                    if (memberAdded) {
                        // Get the created group
                        Group group = database_->getGroupById(groupId);
                        json groupJson;
                        groupJson["id"] = group.id;
                        groupJson["name"] = group.name;
                        groupJson["description"] = group.description;
                        groupJson["creator_id"] = group.creator_id;
                        groupJson["created_at"] = group.created_at;
                        groupJson["is_admin"] = true; // Creator is always admin
                        
                        response = createJSONResponse(true, "Group created successfully", groupJson.dump(), true);
                        std::cout << "[DEBUG] Group creation response sent successfully" << std::endl;
                    } else {
                        response = createErrorResponse("Failed to add creator to group", true);
                        std::cout << "[DEBUG] Failed to add creator to group" << std::endl;
                    }
                } else {
                    response = createErrorResponse("Failed to create group", true);
                    std::cout << "[DEBUG] Failed to create group in database" << std::endl;
                }
            } catch (const json::exception& e) {
                std::cout << "[DEBUG] JSON parsing error: " << e.what() << std::endl;
                response = createErrorResponse("Invalid JSON in request body", true);
            } catch (const std::exception& e) {
                std::cout << "[DEBUG] Exception in group creation: " << e.what() << std::endl;
                response = createErrorResponse("Internal server error during group creation", true);
            }
        }
        else if (method == "GET" && (path == "/groups" || path == "/api/groups")) {
            // Get user's groups
            std::cout << "[DEBUG] Getting groups for user ID: " << userIdInt << std::endl;
            auto groups = groupChat_->getUserGroups(userIdInt);
            std::cout << "[DEBUG] Found " << groups.size() << " groups for user " << userIdInt << std::endl;
            json groupsArray = json::array();
            
            for (const auto& group : groups) {
                json groupJson;
                groupJson["id"] = group.id;
                groupJson["name"] = group.name;
                groupJson["description"] = group.description;
                groupJson["creator_id"] = group.creator_id;
                groupJson["created_at"] = group.created_at;
                groupJson["is_admin"] = groupChat_->isGroupAdmin(group.id, userIdInt);
                
                groupsArray.push_back(groupJson);
            }
            
            response = createJSONResponse(true, "Groups retrieved successfully", groupsArray.dump(), true);
        }
        else if (method == "GET" && (path.find("/groups/") == 0 || path.find("/api/groups/") == 0) && path.find("/members") != std::string::npos) {
            // Get group members
            size_t groupIdStart = path.find("/groups/") != std::string::npos ? path.find("/groups/") + 8 : path.find("/api/groups/") + 12;
            size_t groupIdEnd = path.find("/members");
            int groupId = std::stoi(path.substr(groupIdStart, groupIdEnd - groupIdStart));
            
            if (!groupChat_->isGroupMember(groupId, userIdInt)) {
                response = createErrorResponse("Not a member of this group", true);
                return;
            }
            
            auto members = database_->getGroupMembersWithRole(groupId);
            json membersArray = json::array();
            
            for (const auto& member : members) {
                json memberJson;
                memberJson["id"] = member.id;
                memberJson["username"] = member.username;
                memberJson["email"] = member.email;
                memberJson["role"] = member.role;
                memberJson["is_online"] = member.is_online;
                
                membersArray.push_back(memberJson);
            }
            
            response = createJSONResponse(true, "Group members retrieved successfully", membersArray.dump(), true);
        }
        else if (method == "GET" && (path.find("/groups/") == 0 || path.find("/api/groups/") == 0) && path.find("/members") == std::string::npos && path.find("/messages") == std::string::npos) {
            // Get group info
            size_t groupIdStart = path.find("/groups/") != std::string::npos ? path.find("/groups/") + 8 : path.find("/api/groups/") + 12;
            int groupId = std::stoi(path.substr(groupIdStart));
            
            if (!groupChat_->isGroupMember(groupId, userIdInt)) {
                response = createErrorResponse("Not a member of this group", true);
                return;
            }
            
            Group group = groupChat_->getGroupById(groupId);
            if (group.id == 0) {
                response = createErrorResponse("Group not found", true);
                return;
            }
            
            json groupJson;
            groupJson["id"] = group.id;
            groupJson["name"] = group.name;
            groupJson["description"] = group.description;
            groupJson["creator_id"] = group.creator_id;
            groupJson["created_at"] = group.created_at;
            groupJson["is_admin"] = groupChat_->isGroupAdmin(groupId, userIdInt);
            
            response = createJSONResponse(true, "Group info retrieved successfully", groupJson.dump(), true);
        }
        else if (method == "POST" && (path.find("/groups/") == 0 || path.find("/api/groups/") == 0) && path.find("/members") != std::string::npos) {
            // Invite member to group
            size_t groupIdStart = path.find("/groups/") != std::string::npos ? path.find("/groups/") + 8 : path.find("/api/groups/") + 12;
            size_t groupIdEnd = path.find("/members");
            int groupId = std::stoi(path.substr(groupIdStart, groupIdEnd - groupIdStart));
            
            if (!groupChat_->isGroupAdmin(groupId, userIdInt)) {
                response = createErrorResponse("Not an admin of this group", true);
                return;
            }
            
            json requestJson = json::parse(body);
            std::string username = requestJson["username"];
            std::string role = requestJson.value("role", "member");
            
            // Find user by username
            User user = database_->getUserByUsername(username);
            if (user.id == 0) {
                response = createErrorResponse("User not found", true);
                return;
            }
            
            // Check if user is already a member
            if (groupChat_->isGroupMember(groupId, user.id)) {
                response = createErrorResponse("User is already a member of this group", true);
                return;
            }
            
            // Create invitation instead of directly adding
            std::cout << "[DEBUG] Creating invitation for user " << user.id << " to group " << groupId << " from user " << userIdInt << std::endl;
            bool success = database_->createGroupInvitation(groupId, userIdInt, user.id, role);
            std::cout << "[DEBUG] Invitation creation result: " << (success ? "SUCCESS" : "FAILED") << std::endl;
            
            if (success) {
                response = createJSONResponse(true, "Invitation sent successfully", "", true);
            } else {
                response = createErrorResponse("Failed to send invitation or invitation already exists", true);
            }
        }
        else if (method == "DELETE" && (path.find("/groups/") == 0 || path.find("/api/groups/") == 0) && path.find("/members/") != std::string::npos) {
            // Remove member from group
            size_t groupIdStart = path.find("/groups/") != std::string::npos ? path.find("/groups/") + 8 : path.find("/api/groups/") + 12;
            size_t groupIdEnd = path.find("/members/");
            int groupId = std::stoi(path.substr(groupIdStart, groupIdEnd - groupIdStart));
            
            size_t memberIdStart = groupIdEnd + 9;
            int memberId = std::stoi(path.substr(memberIdStart));
            
            if (!groupChat_->isGroupAdmin(groupId, userIdInt)) {
                response = createErrorResponse("Not an admin of this group", true);
                return;
            }
            
            bool success = groupChat_->removeMember(groupId, memberId, userIdInt);
            
            if (success) {
                response = createJSONResponse(true, "Member removed successfully", "", true);
            } else {
                response = createErrorResponse("Failed to remove member", true);
            }
        }
        else if (method == "POST" && (path.find("/groups/") == 0 || path.find("/api/groups/") == 0) && path.find("/leave") != std::string::npos) {
            // Leave group
            size_t groupIdStart = path.find("/groups/") != std::string::npos ? path.find("/groups/") + 8 : path.find("/api/groups/") + 12;
            size_t groupIdEnd = path.find("/leave");
            int groupId = std::stoi(path.substr(groupIdStart, groupIdEnd - groupIdStart));
            
            if (!groupChat_->isGroupMember(groupId, userIdInt)) {
                response = createErrorResponse("Not a member of this group", true);
                return;
            }
            
            bool success = groupChat_->leaveGroup(groupId, userIdInt);
            
            if (success) {
                response = createJSONResponse(true, "Left group successfully", "", true);
            } else {
                response = createErrorResponse("Failed to leave group", true);
            }
        }
        else if (method == "PUT" && (path.find("/groups/") == 0 || path.find("/api/groups/") == 0)) {
            // Update group
            size_t groupIdStart = path.find("/groups/") != std::string::npos ? path.find("/groups/") + 8 : path.find("/api/groups/") + 12;
            int groupId = std::stoi(path.substr(groupIdStart));
            
            if (!groupChat_->isGroupAdmin(groupId, userIdInt)) {
                response = createErrorResponse("Not an admin of this group", true);
                return;
            }
            
            json requestJson = json::parse(body);
            std::string name = requestJson["name"];
            std::string description = requestJson["description"];
            
            bool success = groupChat_->updateGroup(groupId, name, description, userIdInt);
            
            if (success) {
                response = createJSONResponse(true, "Group updated successfully", "", true);
            } else {
                response = createErrorResponse("Failed to update group", true);
            }
        }
        else if (method == "DELETE" && (path.find("/groups/") == 0 || path.find("/api/groups/") == 0)) {
            // Delete group
            size_t groupIdStart = path.find("/groups/") != std::string::npos ? path.find("/groups/") + 8 : path.find("/api/groups/") + 12;
            int groupId = std::stoi(path.substr(groupIdStart));
            
            if (!groupChat_->isGroupAdmin(groupId, userIdInt)) {
                response = createErrorResponse("Not an admin of this group", true);
                return;
            }
            
            bool success = groupChat_->deleteGroup(groupId, userIdInt);
            
            if (success) {
                response = createJSONResponse(true, "Group deleted successfully", "", true);
            } else {
                response = createErrorResponse("Failed to delete group", true);
            }
        }
        else if (method == "GET" && (path.find("/groups/") == 0 || path.find("/api/groups/") == 0) && path.find("/messages") != std::string::npos) {
            // Get group messages
            size_t groupIdStart = path.find("/groups/") != std::string::npos ? path.find("/groups/") + 8 : path.find("/api/groups/") + 12;
            size_t groupIdEnd = path.find("/messages");
            int groupId = std::stoi(path.substr(groupIdStart, groupIdEnd - groupIdStart));
            
            if (!groupChat_->isGroupMember(groupId, userIdInt)) {
                response = createErrorResponse("Not a member of this group", true);
                return;
            }
            
            auto messages = database_->getGroupMessages(groupId, 50);
            json messagesArray = json::array();
            
            for (const auto& msg : messages) {
                json messageJson;
                messageJson["id"] = msg.id;
                messageJson["content"] = msg.content;
                messageJson["sender_id"] = msg.sender_id;
                messageJson["timestamp"] = msg.timestamp;
                messageJson["is_read"] = msg.is_read;
                messageJson["message_type"] = msg.message_type;
                
                // Get sender name
                User sender = database_->getUserById(msg.sender_id);
                messageJson["sender_name"] = sender.username;
                
                messagesArray.push_back(messageJson);
            }
            
            response = createJSONResponse(true, "Group messages retrieved successfully", messagesArray.dump(), true);
        }
        else if (method == "POST" && (path.find("/groups/") == 0 || path.find("/api/groups/") == 0) && path.find("/messages") != std::string::npos) {
            // Send group message
            size_t groupIdStart = path.find("/groups/") != std::string::npos ? path.find("/groups/") + 8 : path.find("/api/groups/") + 12;
            size_t groupIdEnd = path.find("/messages");
            int groupId = std::stoi(path.substr(groupIdStart, groupIdEnd - groupIdStart));
            
            std::cout << "[DEBUG] Sending group message to group " << groupId << " from user " << userIdInt << std::endl;
            
            bool isMember = groupChat_->isGroupMember(groupId, userIdInt);
            std::cout << "[DEBUG] User " << userIdInt << " is member of group " << groupId << ": " << (isMember ? "YES" : "NO") << std::endl;
            
            if (!isMember) {
                std::cout << "[DEBUG] User " << userIdInt << " is not a member of group " << groupId << std::endl;
                response = createErrorResponse("Not a member of this group", true);
                return;
            }
            
            // Check if this is a file upload (multipart form data)
            auto contentTypeIt = headers.find("content-type");
            bool isFileUpload = false;
            if (contentTypeIt != headers.end() && contentTypeIt->second.find("multipart/form-data") != std::string::npos) {
                isFileUpload = true;
            }
            
            if (isFileUpload) {
                // Handle file upload
                // For now, we'll just save the file info and return success
                // In a real implementation, you'd save the file to disk and store the path
                std::cout << "[DEBUG] File upload detected" << std::endl;
                
                Message message;
                message.sender_id = userIdInt;
                message.group_id = groupId;
                message.receiver_id = 0; // For group messages
                message.content = "File uploaded"; // Placeholder
                message.encrypted_content = "File uploaded"; // Placeholder
                message.message_type = "file"; // or "image" based on file type
                message.file_name = "uploaded_file"; // Extract from multipart data
                message.file_path = "/uploads/file_" + std::to_string(time(nullptr)); // Generate path
                message.file_size = 0; // Extract from multipart data
                
                bool success = database_->saveMessage(message);
                
                std::cout << "[DEBUG] File message save result: " << (success ? "SUCCESS" : "FAILED") << std::endl;
                
                if (success) {
                    response = createJSONResponse(true, "File uploaded successfully", "", true);
                } else {
                    response = createErrorResponse("Failed to upload file", true);
                }
            } else {
                // Handle text message
                json requestJson = json::parse(body);
                std::string content = requestJson["content"];
                std::string messageType = requestJson.value("type", "text");
                
                std::cout << "[DEBUG] Message content: " << content << std::endl;
                
                Message message;
                message.sender_id = userIdInt;
                message.group_id = groupId;
                message.receiver_id = 0; // For group messages
                message.content = content;
                message.encrypted_content = content; // For now, just copy content
                message.message_type = messageType;
                message.file_name = "";
                message.file_path = "";
                message.file_size = 0;
                
                bool success = database_->saveMessage(message);
                
                std::cout << "[DEBUG] Message save result: " << (success ? "SUCCESS" : "FAILED") << std::endl;
                
                if (success) {
                    response = createJSONResponse(true, "Group message sent successfully", "", true);
                } else {
                    response = createErrorResponse("Failed to send group message", true);
                }
            }
        }
        else if (method == "GET" && (path == "/invitations" || path == "/api/invitations")) {
            // Get user's pending invitations
            auto invitations = database_->getPendingInvitations(userIdInt);
            json invitationsArray = json::array();
            
            for (const auto& invitation : invitations) {
                json invitationJson;
                invitationJson["id"] = invitation.id;
                invitationJson["group_id"] = invitation.group_id;
                invitationJson["inviter_id"] = invitation.inviter_id;
                invitationJson["invitee_id"] = invitation.invitee_id;
                invitationJson["role"] = invitation.role;
                invitationJson["status"] = invitation.status;
                invitationJson["created_at"] = invitation.created_at;
                invitationJson["expires_at"] = invitation.expires_at;
                invitationJson["responded_at"] = invitation.responded_at;
                
                // Get group info
                Group group = database_->getGroupById(invitation.group_id);
                invitationJson["group_name"] = group.name;
                invitationJson["group_description"] = group.description;
                
                // Get inviter info
                User inviter = database_->getUserById(invitation.inviter_id);
                invitationJson["inviter_username"] = inviter.username;
                
                invitationsArray.push_back(invitationJson);
            }
            
            response = createJSONResponse(true, "Invitations retrieved successfully", invitationsArray.dump(), true);
        }
        else if (method == "POST" && (path.find("/invitations/") == 0 || path.find("/api/invitations/") == 0) && path.find("/accept") != std::string::npos) {
            // Accept invitation
            std::cout << "[DEBUG] Accept invitation route reached" << std::endl;
            std::cout << "[DEBUG] Path: " << path << std::endl;
            std::cout << "[DEBUG] About to parse invitation ID..." << std::endl;
            size_t invitationIdStart = path.find("/invitations/") != std::string::npos ? path.find("/invitations/") + 12 : path.find("/api/invitations/") + 16;
            size_t invitationIdEnd = path.find("/accept");
            std::cout << "[DEBUG] invitationIdStart: " << invitationIdStart << ", invitationIdEnd: " << invitationIdEnd << std::endl;
            std::string invitationIdStr = path.substr(invitationIdStart, invitationIdEnd - invitationIdStart);
            if (!invitationIdStr.empty() && invitationIdStr[0] == '/') {
                invitationIdStr = invitationIdStr.substr(1);
            }
            std::cout << "[DEBUG] invitationIdStr: '" << invitationIdStr << "'" << std::endl;
            int invitationId = std::stoi(invitationIdStr);
            
            std::cout << "[DEBUG] Accept invitation: invitationId=" << invitationId << ", userIdInt=" << userIdInt << std::endl;
            
            bool success = database_->acceptGroupInvitation(invitationId, userIdInt);
            
            std::cout << "[DEBUG] Accept invitation result: " << (success ? "SUCCESS" : "FAILED") << std::endl;
            
            if (success) {
                response = createJSONResponse(true, "Invitation accepted successfully", "", true);
            } else {
                response = createErrorResponse("Failed to accept invitation", true);
            }
        }
        else if (method == "POST" && (path.find("/invitations/") == 0 || path.find("/api/invitations/") == 0) && path.find("/decline") != std::string::npos) {
            // Decline invitation
            size_t invitationIdStart = path.find("/invitations/") != std::string::npos ? path.find("/invitations/") + 12 : path.find("/api/invitations/") + 16;
            size_t invitationIdEnd = path.find("/decline");
            std::string invitationIdStr = path.substr(invitationIdStart, invitationIdEnd - invitationIdStart);
            if (!invitationIdStr.empty() && invitationIdStr[0] == '/') {
                invitationIdStr = invitationIdStr.substr(1);
            }
            int invitationId = std::stoi(invitationIdStr);
            
            bool success = database_->declineGroupInvitation(invitationId, userIdInt);
            
            if (success) {
                response = createJSONResponse(true, "Invitation declined successfully", "", true);
            } else {
                response = createErrorResponse("Failed to decline invitation", true);
            }
        }
        else {
            response = createErrorResponse("Method not allowed", true);
        }
    } catch (const std::exception& e) {
        response = createErrorResponse("Internal server error: " + std::string(e.what()), true);
    }
}

void Server::handleBackupRoutes(const std::string& method, const std::string& path, const std::string& body, const std::map<std::string, std::string>& headers, std::string& response) {
    try {
        int userIdInt = 0;
        auto authIt = headers.find("Authorization");
        if (authIt != headers.end()) {
            std::string authHeader = authIt->second;
            if (authHeader.find("Bearer ") == 0) {
                std::string token = authHeader.substr(7);
                if (!userManager_->validateSessionToken(token, userIdInt)) {
                    response = createErrorResponse("Unauthorized", true);
                    return;
                }
            }
        }
        
        if (method == "POST" && path == "/backup") {
            // Create chat backup
            json requestJson = json::parse(body);
            std::string backupName = requestJson["name"];
            std::string backupData = requestJson["data"];
            std::string description = requestJson.value("description", "");
            
            bool success = database_->createChatBackup(userIdInt, backupName, backupData, description);
            
            if (success) {
                response = createJSONResponse(true, "Backup created successfully", "", true);
            } else {
                response = createErrorResponse("Failed to create backup", true);
            }
        }
        else if (method == "GET" && path == "/backup") {
            // Get user's backups
            auto backups = database_->getUserBackups(userIdInt);
            json backupsArray = json::array();
            
            for (const auto& backup : backups) {
                json backupJson;
                backupJson["id"] = backup.id;
                backupJson["name"] = backup.backup_name;
                backupJson["description"] = backup.description;
                backupJson["created_at"] = backup.created_at;
                backupJson["size"] = backup.backup_data.length();
                
                backupsArray.push_back(backupJson);
            }
            
            response = createJSONResponse(true, "Backups retrieved successfully", backupsArray.dump(), true);
        }
        else if (method == "GET" && path.find("/backup/") == 0) {
            // Get specific backup
            size_t backupIdStart = path.find("/backup/") + 8;
            int backupId = std::stoi(path.substr(backupIdStart));
            
            auto backup = database_->getBackupById(backupId);
            if (backup.id == 0 || backup.user_id != userIdInt) {
                response = createErrorResponse("Backup not found", true);
                return;
            }
            
            json backupJson;
            backupJson["id"] = backup.id;
            backupJson["name"] = backup.backup_name;
            backupJson["description"] = backup.description;
            backupJson["created_at"] = backup.created_at;
            backupJson["data"] = backup.backup_data;
            
            response = createJSONResponse(true, "Backup retrieved successfully", backupJson.dump(), true);
        }
        else if (method == "POST" && path.find("/backup/") == 0 && path.find("/restore") != std::string::npos) {
            // Restore from backup
            size_t backupIdStart = path.find("/backup/") + 8;
            size_t backupIdEnd = path.find("/restore");
            int backupId = std::stoi(path.substr(backupIdStart, backupIdEnd - backupIdStart));
            
            bool success = database_->restoreFromBackup(backupId, userIdInt);
            
            if (success) {
                response = createJSONResponse(true, "Backup restored successfully", "", true);
            } else {
                response = createErrorResponse("Failed to restore backup", true);
            }
        }
        else if (method == "DELETE" && path.find("/backup/") == 0) {
            // Delete backup
            size_t backupIdStart = path.find("/backup/") + 8;
            int backupId = std::stoi(path.substr(backupIdStart));
            
            bool success = database_->deleteBackup(backupId, userIdInt);
            
            if (success) {
                response = createJSONResponse(true, "Backup deleted successfully", "", true);
            } else {
                response = createErrorResponse("Failed to delete backup", true);
            }
        }
        else {
            response = createErrorResponse("Method not allowed", true);
        }
    } catch (const std::exception& e) {
        response = createErrorResponse("Internal server error: " + std::string(e.what()), true);
    }
}

void Server::handleChatSessionRoutes(const std::string& method, const std::string& path, const std::string& body, const std::map<std::string, std::string>& headers, std::string& response) {
    std::cout << "[DEBUG] handleChatSessionRoutes called with method: " << method << ", path: " << path << std::endl;
    std::cout << "[DEBUG] Request body: " << body << std::endl;
    
    int userIdInt = 0; // Default for development
    // Accept both 'Authorization' and 'authorization' headers (case-insensitive)
    auto authIt = headers.find("Authorization");
    if (authIt == headers.end()) {
        authIt = headers.find("authorization");
    }
    if (authIt != headers.end()) {
        std::string authHeader = authIt->second;
        if (authHeader.find("Bearer ") == 0) {
            std::string token = authHeader.substr(7);
            if (userManager_->validateSessionToken(token, userIdInt)) {
                std::cout << "[DEBUG] Token validation successful for user: " << userIdInt << std::endl;
            } else {
                std::cout << "[DEBUG] Token validation failed" << std::endl;
                response = createErrorResponse("Invalid token", true);
                return;
            }
        }
    } else {
        std::cout << "[DEBUG] No Authorization header found" << std::endl;
    }
    
    if (method == "GET" && path == "/chat-sessions") {
        std::cout << "[DEBUG] Getting chat sessions for user: " << userIdInt << std::endl;
        // Get user's chat sessions
        auto sessions = database_->getUserChatSessions(userIdInt);
        json sessionsArray = json::array();
        for (const auto& session : sessions) {
            json sessionJson;
            sessionJson["id"] = session.id;
            sessionJson["other_user_id"] = session.other_user_id;
            sessionJson["group_id"] = session.group_id;
            sessionJson["last_message"] = session.last_message;
            sessionJson["last_timestamp"] = session.last_timestamp;
            sessionJson["unread_count"] = session.unread_count;
            sessionJson["updated_at"] = session.updated_at;
            if (session.other_user_id > 0) {
                User otherUser = database_->getUserById(session.other_user_id);
                sessionJson["other_user_name"] = otherUser.username;
            }
            sessionsArray.push_back(sessionJson);
        }
        response = createJSONResponse(true, "Chat sessions retrieved successfully", sessionsArray.dump(), true);
    } else if (method == "POST" && path == "/api/chat-sessions") {
        // Create new chat session
        std::cout << "[DEBUG] Creating chat session" << std::endl;
        std::cout << "[DEBUG] Request body: " << body << std::endl;
        
        try {
            json requestJson = json::parse(body);
            int otherUserId = requestJson["other_user_id"];
            std::cout << "[DEBUG] Other user ID: " << otherUserId << std::endl;
            std::cout << "[DEBUG] Current user ID: " << userIdInt << std::endl;
            
            // Check if other user exists
            User otherUser = userManager_->getUserById(otherUserId);
            std::cout << "[DEBUG] Other user lookup result - ID: " << otherUser.id << ", Username: " << otherUser.username << std::endl;
            if (otherUser.id == 0) {
                std::cout << "[DEBUG] User not found" << std::endl;
                response = createErrorResponse("User not found", true);
                return;
            }
            
            // Check if chat session already exists
            auto existingSessions = database_->getUserChatSessions(userIdInt);
            std::cout << "[DEBUG] Found " << existingSessions.size() << " existing sessions" << std::endl;
            for (const auto& session : existingSessions) {
                std::cout << "[DEBUG] Checking session with other_user_id: " << session.other_user_id << std::endl;
                if (session.other_user_id == otherUserId) {
                    std::cout << "[DEBUG] Chat session already exists" << std::endl;
                    response = createErrorResponse("Chat session already exists", true);
                    return;
                }
            }
            
            // Create new chat session
            std::cout << "[DEBUG] Attempting to create chat session..." << std::endl;
            bool success = database_->createOrUpdateChatSession(userIdInt, otherUserId, 0, "", 0);
            std::cout << "[DEBUG] Chat session creation result: " << (success ? "SUCCESS" : "FAILED") << std::endl;
            
            if (success) {
                // Also create session for the other user
                std::cout << "[DEBUG] Creating reverse session..." << std::endl;
                bool reverseSuccess = database_->createOrUpdateChatSession(otherUserId, userIdInt, 0, "", 0);
                std::cout << "[DEBUG] Reverse session creation result: " << (reverseSuccess ? "SUCCESS" : "FAILED") << std::endl;
                
                json sessionJson;
                sessionJson["id"] = 0; // Will be set by database
                sessionJson["other_user_id"] = otherUserId;
                sessionJson["last_message"] = "";
                sessionJson["unread_count"] = 0;
                
                std::cout << "[DEBUG] Returning success response" << std::endl;
                response = createJSONResponse(true, "Chat session created successfully", sessionJson.dump(), true);
            } else {
                std::cout << "[DEBUG] Returning failure response" << std::endl;
                response = createErrorResponse("Failed to create chat session", true);
            }
        } catch (const json::exception& e) {
            std::cout << "[DEBUG] JSON parsing error: " << e.what() << std::endl;
            response = createErrorResponse("Invalid JSON in request body", true);
        } catch (const std::exception& e) {
            std::cout << "[DEBUG] Exception in chat session creation: " << e.what() << std::endl;
            response = createErrorResponse("Internal server error during chat session creation", true);
        }
    } else if (method == "GET" && (path.find("/chat-sessions/") == 0 || path.find("/api/chat-sessions/") == 0) && path.find("/messages") != std::string::npos) {
        // Get messages for a specific chat session
        std::cout << "[DEBUG] Getting messages for chat session" << std::endl;
        
        // Extract session ID from path
        size_t sessionIdStart = path.find("/chat-sessions/") != std::string::npos ? path.find("/chat-sessions/") + 14 : path.find("/api/chat-sessions/") + 19;
        size_t sessionIdEnd = path.find("/messages");
        std::string sessionIdStr = path.substr(sessionIdStart, sessionIdEnd - sessionIdStart);
        int sessionId = std::stoi(sessionIdStr);
        
        std::cout << "[DEBUG] Session ID: " << sessionId << std::endl;
        
        // Verify user has access to this chat session and get the other user ID
        auto sessions = database_->getUserChatSessions(userIdInt);
        bool hasAccess = false;
        int otherUserId = 0;
        for (const auto& session : sessions) {
            if (session.id == sessionId) {
                hasAccess = true;
                otherUserId = session.other_user_id;
                break;
            }
        }
        
        if (!hasAccess) {
            std::cout << "[DEBUG] User does not have access to session " << sessionId << std::endl;
            response = createErrorResponse("Access denied to this chat session", true);
            return;
        }
        
        std::cout << "[DEBUG] Other user ID: " << otherUserId << std::endl;
        
        // Get messages for this session using the other user ID
        auto messages = database_->getMessages(userIdInt, otherUserId, 50);
        json messagesArray = json::array();
        
        for (const auto& msg : messages) {
            json messageJson;
            messageJson["id"] = msg.id;
            messageJson["content"] = msg.content;
            messageJson["sender_id"] = msg.sender_id;
            messageJson["timestamp"] = msg.timestamp;
            messageJson["is_read"] = msg.is_read;
            messageJson["message_type"] = msg.message_type;
            
            // Get sender name
            User sender = database_->getUserById(msg.sender_id);
            messageJson["sender_name"] = sender.username;
            
            messagesArray.push_back(messageJson);
        }
        
        std::cout << "[DEBUG] Returning " << messagesArray.size() << " messages" << std::endl;
        response = createJSONResponse(true, "Messages retrieved successfully", messagesArray.dump(), true);
        std::cout << "[DEBUG] Response: " << response << std::endl;
    } else if (method == "POST" && (path.find("/chat-sessions/") == 0 || path.find("/api/chat-sessions/") == 0) && path.find("/messages") != std::string::npos) {
        // Send a message in a specific chat session
        std::cout << "[DEBUG] Sending message in chat session" << std::endl;
        std::cout << "[DEBUG] Request body: " << body << std::endl;
        try {
            // Extract session ID from path
            size_t sessionIdStart = path.find("/chat-sessions/") != std::string::npos ? path.find("/chat-sessions/") + 14 : path.find("/api/chat-sessions/") + 19;
            size_t sessionIdEnd = path.find("/messages");
            std::string sessionIdStr = path.substr(sessionIdStart, sessionIdEnd - sessionIdStart);
            int sessionId = std::stoi(sessionIdStr);
            std::cout << "[DEBUG] Session ID: " << sessionId << std::endl;

            // Verify user has access to this chat session
            auto sessions = database_->getUserChatSessions(userIdInt);
            bool hasAccess = false;
            int otherUserId = 0;
            for (const auto& session : sessions) {
                if (session.id == sessionId) {
                    hasAccess = true;
                    otherUserId = session.other_user_id;
                    break;
                }
            }
            std::cout << "[DEBUG] hasAccess: " << hasAccess << ", otherUserId: " << otherUserId << std::endl;
            if (!hasAccess || otherUserId == 0) {
                std::cout << "[DEBUG] User does not have access to session or invalid otherUserId" << std::endl;
                response = createErrorResponse("Access denied to this chat session", true);
                std::cout << "[DEBUG] Response: " << response << std::endl;
                return;
            }

            try {
                json requestJson = json::parse(body);
                std::string content = requestJson["content"];
                std::string messageType = requestJson.value("type", "text");
                std::cout << "[DEBUG] Message content: " << content << ", messageType: " << messageType << std::endl;
                std::cout << "[DEBUG] userIdInt: " << userIdInt << ", otherUserId: " << otherUserId << std::endl;

                // Create and save the message
                Message message;
                message.sender_id = userIdInt;
                message.receiver_id = otherUserId;
                message.content = content;
                message.message_type = messageType;
                std::cout << "[DEBUG] About to call saveMessage" << std::endl;
                bool success = database_->saveMessage(message);
                std::cout << "[DEBUG] saveMessage result: " << (success ? "SUCCESS" : "FAILED") << std::endl;
                if (success) {
                    // Update the chat session with the new message
                    database_->createOrUpdateChatSession(userIdInt, otherUserId, 0, content, 0);
                    database_->createOrUpdateChatSession(otherUserId, userIdInt, 0, content, 1); // Increment unread count for other user
                    std::cout << "[DEBUG] Message sent successfully" << std::endl;
                    response = createJSONResponse(true, "Message sent successfully", "", true);
                } else {
                    std::cout << "[DEBUG] Failed to send message" << std::endl;
                    response = createErrorResponse("Failed to send message", true);
                }
                std::cout << "[DEBUG] Response: " << response << std::endl;
            } catch (const json::exception& e) {
                std::cout << "[DEBUG] JSON parsing error: " << e.what() << std::endl;
                response = createErrorResponse("Invalid JSON in request body", true);
                std::cout << "[DEBUG] Response: " << response << std::endl;
            } catch (const std::exception& e) {
                std::cout << "[DEBUG] Exception in message sending: " << e.what() << std::endl;
                response = createErrorResponse("Internal server error during message sending", true);
                std::cout << "[DEBUG] Response: " << response << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "[DEBUG] Outer exception in POST /chat-sessions/{{id}}/messages: " << e.what() << std::endl;
            response = createErrorResponse("Internal server error during chat session lookup", true);
            std::cout << "[DEBUG] Response: " << response << std::endl;
        }
    } else {
        std::cout << "[DEBUG] Method not allowed: " << method << " " << path << std::endl;
        response = createErrorResponse("Method not allowed", true);
    }
}

void Server::parseRequest(const std::string& request, std::string& method, std::string& path, std::map<std::string, std::string>& headers, std::string& body) {
    std::istringstream requestStream(request);
    std::string line;
    
    // Parse request line
    if (std::getline(requestStream, line)) {
        std::istringstream lineStream(line);
        lineStream >> method >> path;
    }
    
    // Parse headers
    while (std::getline(requestStream, line) && line != "\r" && !line.empty()) {
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            
            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t\r\n"));
            key.erase(key.find_last_not_of(" \t\r\n") + 1);
            value.erase(0, value.find_first_not_of(" \t\r\n"));
            value.erase(value.find_last_not_of(" \t\r\n") + 1);
            
            headers[key] = value;
        }
    }
    
    // Get body
    std::stringstream bodyStream;
    while (std::getline(requestStream, line)) {
        bodyStream << line << "\n";
    }
    body = bodyStream.str();
    
    // Remove trailing newline
    if (!body.empty() && body.back() == '\n') {
        body.pop_back();
    }
}

void Server::handleRequest(const std::string& request, std::string& response) {
    try {
        std::string method, path;
        std::map<std::string, std::string> headers;
        std::string body;
        
        parseRequest(request, method, path, headers, body);
        
        std::cout << "[DEBUG] handleRequest: method=" << method << ", path=" << path << std::endl;
        
        // Find the route handler using prefix matching
        std::function<void(const std::string&, const std::string&, const std::string&, const std::map<std::string, std::string>&, std::string&)> routeHandler = nullptr;
        std::string matchedPrefix;
        
        // Sort routes by length (longest first) to ensure more specific routes are matched
        std::vector<std::pair<std::string, std::function<void(const std::string&, const std::string&, const std::string&, const std::map<std::string, std::string>&, std::string&)>>> sortedRoutes;
        for (const auto& [prefix, handler] : routes) {
            sortedRoutes.push_back({prefix, handler});
        }
        
        std::sort(sortedRoutes.begin(), sortedRoutes.end(), 
            [](const auto& a, const auto& b) {
                return a.first.length() > b.first.length();
            });
        
        for (const auto& [prefix, handler] : sortedRoutes) {
            if (path.substr(0, prefix.length()) == prefix) {
                matchedPrefix = prefix;
                routeHandler = handler;
                break; // Use the first (longest) match
            }
        }
        
        std::cout << "[DEBUG] Matched prefix: " << matchedPrefix << std::endl;
        
        if (routeHandler) {
            routeHandler(method, path, body, headers, response);
        } else {
            response = createErrorResponse("Route not found", true);
        }
        
        // Add CORS headers
        addCORSHeaders(response);
        
    } catch (const std::exception& e) {
        response = createErrorResponse("Internal server error: " + std::string(e.what()), true);
    }
}

void Server::handleOAuthCallbackRoutes(const std::string& method, const std::string& path, const std::string& body, const std::map<std::string, std::string>& headers, std::string& response) {
    try {
        if (method == "GET" && path == "/oauth/gmail/callback") {
            // Parse query parameters from the request
            std::string queryString;
            size_t queryStart = body.find("?");
            if (queryStart != std::string::npos) {
                queryString = body.substr(queryStart + 1);
            }
            
            // Extract authorization code from query parameters
            std::string code;
            size_t codeStart = queryString.find("code=");
            if (codeStart != std::string::npos) {
                size_t codeEnd = queryString.find("&", codeStart);
                if (codeEnd != std::string::npos) {
                    code = queryString.substr(codeStart + 5, codeEnd - codeStart - 5);
                } else {
                    code = queryString.substr(codeStart + 5);
                }
            }
            
            if (code.empty()) {
                // Return an HTML page with error
                response = "HTTP/1.1 200 OK\r\n";
                response += "Content-Type: text/html\r\n";
                response += "\r\n";
                response += "<html><body>";
                response += "<h2>Gmail OAuth2 Error</h2>";
                response += "<p>No authorization code received. Please try again.</p>";
                response += "<script>window.close();</script>";
                response += "</body></html>";
                return;
            }
            
            // Return an HTML page with the authorization code
            response = "HTTP/1.1 200 OK\r\n";
            response += "Content-Type: text/html\r\n";
            response += "\r\n";
            response += "<html><body>";
            response += "<h2>Gmail OAuth2 Success</h2>";
            response += "<p>Authorization successful! You can close this window.</p>";
            response += "<p>Authorization Code: <code>" + code + "</code></p>";
            response += "<p>Copy this code and paste it in the Cockpit application.</p>";
            response += "<script>";
            response += "setTimeout(function() { window.close(); }, 10000);";
            response += "</script>";
            response += "</body></html>";
        } else {
            response = createErrorResponse("Method not allowed", true);
        }
    } catch (const std::exception& e) {
        response = createErrorResponse("Internal server error: " + std::string(e.what()), true);
    }
} 