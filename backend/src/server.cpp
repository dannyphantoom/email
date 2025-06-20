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

using json = nlohmann::json;

Server::Server(int port) : port_(port), running_(false), database_(std::make_shared<Database>()), 
                          userManager_(std::make_shared<UserManager>(database_)),
                          messageHandler_(std::make_shared<MessageHandler>(database_, userManager_)),
                          wsHandler_(std::make_shared<WebSocketHandler>(messageHandler_, userManager_)) {
    setupRoutes();
}

Server::~Server() {
    stop();
}

void Server::setupRoutes() {
    routes["/auth/register"] = [this](const std::string& method, const std::string& path, const std::string& body, std::string& response) {
        handleAuthRoutes(method, path, body, response);
    };
    routes["/auth/login"] = [this](const std::string& method, const std::string& path, const std::string& body, std::string& response) {
        handleAuthRoutes(method, path, body, response);
    };
    routes["/auth/logout"] = [this](const std::string& method, const std::string& path, const std::string& body, std::string& response) {
        handleAuthRoutes(method, path, body, response);
    };
    routes["/users"] = [this](const std::string& method, const std::string& path, const std::string& body, std::string& response) {
        handleUserRoutes(method, path, body, response);
    };
    routes["/messages"] = [this](const std::string& method, const std::string& path, const std::string& body, std::string& response) {
        handleMessageRoutes(method, path, body, response);
    };
    routes["/groups"] = [this](const std::string& method, const std::string& path, const std::string& body, std::string& response) {
        handleGroupRoutes(method, path, body, response);
    };
    
    // Account integration routes
    routes["/integration/accounts"] = [this](const std::string& method, const std::string& path, const std::string& body, std::string& response) {
        handleAccountIntegrationRoutes(method, path, body, response);
    };
    routes["/integration/connect/gmail"] = [this](const std::string& method, const std::string& path, const std::string& body, std::string& response) {
        handleAccountIntegrationRoutes(method, path, body, response);
    };
    routes["/integration/connect/gmail/oauth2"] = [this](const std::string& method, const std::string& path, const std::string& body, std::string& response) {
        handleAccountIntegrationRoutes(method, path, body, response);
    };
    routes["/integration/gmail/oauth2/url"] = [this](const std::string& method, const std::string& path, const std::string& body, std::string& response) {
        handleAccountIntegrationRoutes(method, path, body, response);
    };
    routes["/integration/connect/whatsapp"] = [this](const std::string& method, const std::string& path, const std::string& body, std::string& response) {
        handleAccountIntegrationRoutes(method, path, body, response);
    };
    routes["/integration/messages"] = [this](const std::string& method, const std::string& path, const std::string& body, std::string& response) {
        handleAccountIntegrationRoutes(method, path, body, response);
    };
    routes["/integration/sync"] = [this](const std::string& method, const std::string& path, const std::string& body, std::string& response) {
        handleAccountIntegrationRoutes(method, path, body, response);
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
                // No connection available, continue
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
std::string Server::createJSONResponse(bool success, const std::string& message, const std::string& data) {
    json response;
    response["success"] = success;
    response["message"] = message;
    if (!data.empty()) {
        response["data"] = json::parse(data);
    }
    return response.dump();
}

std::string Server::createErrorResponse(const std::string& error) {
    return createJSONResponse(false, error);
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

bool Server::validateToken(const std::string& token, std::string& userId) {
    // TODO: Implement proper JWT token validation
    // For now, return true if token is not empty
    if (!token.empty()) {
        userId = "user123"; // Mock user ID
        return true;
    }
    return false;
}

void Server::handleAccountIntegrationRoutes(const std::string& method, const std::string& path, const std::string& body, std::string& response) {
    try {
        // Parse headers from the request (this would need to be passed from the calling method)
        std::map<std::string, std::string> headers;
        // TODO: Extract headers from the actual request
        
        if (method == "GET" && path == "/integration/accounts") {
            // Get user's connected accounts
            std::string userId = getAuthToken(headers);
            if (userId.empty()) {
                response = createErrorResponse("Unauthorized");
                return;
            }
            
            auto accounts = accountManager.getUserAccounts(userId);
            json accountsArray = json::array();
            
            for (const auto& account : accounts) {
                json accountJson;
                accountJson["id"] = account.id;
                accountJson["type"] = (account.type == AccountType::EMAIL) ? "email" : "messenger";
                accountJson["provider"] = [&account]() {
                    switch (account.provider) {
                        case ProviderType::GMAIL: return "Gmail";
                        case ProviderType::OUTLOOK: return "Outlook";
                        case ProviderType::YAHOO_MAIL: return "Yahoo Mail";
                        case ProviderType::PROTONMAIL: return "ProtonMail";
                        case ProviderType::WHATSAPP: return "WhatsApp";
                        case ProviderType::TELEGRAM: return "Telegram";
                        case ProviderType::FACEBOOK_MESSENGER: return "Facebook Messenger";
                        case ProviderType::TWITTER_DM: return "Twitter DM";
                        case ProviderType::INSTAGRAM_DM: return "Instagram DM";
                        default: return "Unknown";
                    }
                }();
                accountJson["email"] = account.email;
                accountJson["username"] = account.username;
                accountJson["isActive"] = account.isActive;
                accountJson["lastSync"] = std::chrono::duration_cast<std::chrono::seconds>(
                    account.lastSync.time_since_epoch()).count();
                
                accountsArray.push_back(accountJson);
            }
            
            response = createJSONResponse(true, "Accounts retrieved successfully", accountsArray.dump());
        }
        else if (method == "POST" && path == "/integration/connect/gmail") {
            // Connect Gmail account
            std::string userId = getAuthToken(headers);
            if (userId.empty()) {
                response = createErrorResponse("Unauthorized");
                return;
            }
            
            json requestJson = json::parse(body);
            std::string email = requestJson["email"];
            std::string password = requestJson["password"];
            
            bool success = accountManager.connectGmail(userId, email, password);
            
            if (success) {
                response = createJSONResponse(true, "Gmail account connected successfully");
            } else {
                response = createErrorResponse("Failed to connect Gmail account");
            }
        }
        else if (method == "GET" && path == "/integration/gmail/oauth2/url") {
            // Return Gmail OAuth2 authorization URL
            std::string url = accountManager.getGmailOAuth2Url();
            json data;
            data["url"] = url;
            response = createJSONResponse(true, "Gmail OAuth2 URL generated", data.dump());
        }
        else if (method == "POST" && path == "/integration/connect/gmail/oauth2") {
            // Complete Gmail OAuth2 connection using authorization code
            std::string userId = getAuthToken(headers);
            if (userId.empty()) {
                response = createErrorResponse("Unauthorized");
                return;
            }

            json requestJson = json::parse(body);
            std::string email = requestJson["email"];
            std::string code = requestJson["code"];

            std::string accessToken;
            std::string refreshToken;
            if (!accountManager.exchangeGmailCodeForTokens(code, accessToken, refreshToken)) {
                response = createErrorResponse("Failed to exchange OAuth2 code");
                return;
            }

            bool success = accountManager.connectGmailOAuth2(userId, email, accessToken, refreshToken);
            if (success) {
                response = createJSONResponse(true, "Gmail account connected via OAuth2");
            } else {
                response = createErrorResponse("Failed to connect Gmail via OAuth2");
            }
        }
        else if (method == "POST" && path == "/integration/connect/whatsapp") {
            // Connect WhatsApp account
            std::string userId = getAuthToken(headers);
            if (userId.empty()) {
                response = createErrorResponse("Unauthorized");
                return;
            }
            
            json requestJson = json::parse(body);
            std::string phoneNumber = requestJson["phoneNumber"];
            std::string password = requestJson["password"];
            
            bool success = accountManager.connectWhatsApp(userId, phoneNumber, password);
            
            if (success) {
                response = createJSONResponse(true, "WhatsApp account connected successfully");
            } else {
                response = createErrorResponse("Failed to connect WhatsApp account");
            }
        }
        else if (method == "GET" && path == "/integration/messages") {
            // Get unified messages
            std::string userId = getAuthToken(headers);
            if (userId.empty()) {
                response = createErrorResponse("Unauthorized");
                return;
            }
            
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
                messageJson["attachments"] = message.attachments;
                
                messagesArray.push_back(messageJson);
            }
            
            response = createJSONResponse(true, "Messages retrieved successfully", messagesArray.dump());
        }
        else if (method == "POST" && path == "/integration/sync") {
            // Manual sync trigger
            std::string userId = getAuthToken(headers);
            if (userId.empty()) {
                response = createErrorResponse("Unauthorized");
                return;
            }
            
            json requestJson = json::parse(body);
            std::string accountId = requestJson["accountId"];
            
            bool success = accountManager.syncAccount(userId, accountId);
            
            if (success) {
                response = createJSONResponse(true, "Account synced successfully");
            } else {
                response = createErrorResponse("Failed to sync account");
            }
        }
        else {
            response = createErrorResponse("Method not allowed");
        }
    } catch (const std::exception& e) {
        response = createErrorResponse("Internal server error: " + std::string(e.what()));
    }
}

void Server::handleAuthRoutes(const std::string& method, const std::string& path, const std::string& body, std::string& response) {
    response = createErrorResponse("Not implemented");
}

void Server::handleUserRoutes(const std::string& method, const std::string& path, const std::string& body, std::string& response) {
    response = createErrorResponse("Not implemented");
}

void Server::handleMessageRoutes(const std::string& method, const std::string& path, const std::string& body, std::string& response) {
    response = createErrorResponse("Not implemented");
}

void Server::handleGroupRoutes(const std::string& method, const std::string& path, const std::string& body, std::string& response) {
    response = createErrorResponse("Not implemented");
} 