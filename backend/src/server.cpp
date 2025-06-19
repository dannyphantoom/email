#include "server.h"
#include "database.h"
#include "user_manager.h"
#include "message_handler.h"
#include "websocket_handler.h"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <cstring>

Server::Server(int port) : port_(port), serverSocket_(-1), running_(false) {
    // Initialize components
    database_ = std::make_shared<Database>();
    userManager_ = std::make_shared<UserManager>(database_);
    messageHandler_ = std::make_shared<MessageHandler>(database_, userManager_);
    wsHandler_ = std::make_shared<WebSocketHandler>(messageHandler_, userManager_);
}

Server::~Server() {
    stop();
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