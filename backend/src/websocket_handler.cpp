#include "websocket_handler.h"
#include "message_handler.h"
#include "user_manager.h"
#include "server.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <sys/select.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

WebSocketHandler::WebSocketHandler(std::shared_ptr<MessageHandler> msgHandler, 
                                 std::shared_ptr<UserManager> userManager,
                                 Server* server)
    : messageHandler_(msgHandler), userManager_(userManager), server_(server) {
}

WebSocketHandler::~WebSocketHandler() {
    // Clean up connections
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    for (auto& [key, conn] : connections_) {
        if (conn && conn->active) {
            ::close(conn->socket);
        }
    }
}

void WebSocketHandler::handleConnection(int clientSocket, const std::string& remoteAddress) {
    auto connection = std::make_shared<WebSocketConnection>(clientSocket, remoteAddress);
    
    // Start a new thread to handle this connection
    connection->read_thread = std::thread([this, connection]() {
        handleClient(connection);
    });
    connection->read_thread.detach(); // Detach the thread to avoid SIGABRT
    
    // Store connection (will be moved to authenticated connections after login)
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    connections_[clientSocket] = connection;
}

void WebSocketHandler::handleClient(std::shared_ptr<WebSocketConnection> conn) {
    if (!conn) {
        std::cerr << "Invalid connection object" << std::endl;
        return;
    }

    char buffer[4096];
    std::string request;
    
    try {
        // Read HTTP request with timeout
        fd_set readfds;
        struct timeval timeout;
        timeout.tv_sec = 5;  // 5 second timeout
        timeout.tv_usec = 0;
        
        FD_ZERO(&readfds);
        FD_SET(conn->socket, &readfds);
        
        int select_result = select(conn->socket + 1, &readfds, NULL, NULL, &timeout);
        if (select_result <= 0) {
            std::cerr << "Timeout or error waiting for data" << std::endl;
            ::close(conn->socket);
            return;
        }
        
        // Read HTTP request
        while (true) {
            ssize_t bytesRead = recv(conn->socket, buffer, sizeof(buffer) - 1, 0);
            if (bytesRead <= 0) {
                if (bytesRead == 0) {
                    std::cout << "Client disconnected" << std::endl;
                } else {
                    std::cerr << "Error reading from socket: " << strerror(errno) << std::endl;
                }
                ::close(conn->socket);
                return;
            }
            
            buffer[bytesRead] = '\0';
            request += buffer;
            
            if (request.find("\r\n\r\n") != std::string::npos) {
                break; // End of HTTP request
            }
            
            // Prevent infinite loop
            if (request.length() > 8192) {
                std::cerr << "Request too large" << std::endl;
                sendErrorResponse(conn, 413, "Request Entity Too Large");
                return;
            }
        }
        
        // Validate request format
        if (request.empty() || request.find("HTTP/") == std::string::npos) {
            std::cerr << "Invalid HTTP request format" << std::endl;
            sendErrorResponse(conn, 400, "Bad Request");
            return;
        }
        
        // Parse HTTP request
        std::string method, path, version;
        std::istringstream requestStream(request);
        requestStream >> method >> path >> version;
        
        if (method.empty() || path.empty() || version.empty()) {
            std::cerr << "Invalid HTTP request line" << std::endl;
            sendErrorResponse(conn, 400, "Bad Request");
            return;
        }
        
        std::cout << "HTTP Request: " << method << " " << path << std::endl;
        
        // Check if this is a WebSocket upgrade request
        if (request.find("Upgrade: websocket") != std::string::npos) {
            if (performHandshake(conn->socket, request)) {
                std::cout << "WebSocket handshake successful for " << conn->remote_address << std::endl;
                
                // Handle WebSocket messages
                while (conn->active) {
                    ssize_t bytesRead = recv(conn->socket, buffer, sizeof(buffer), 0);
                    if (bytesRead <= 0) {
                        break;
                    }
                    
                    std::string data(buffer, bytesRead);
                    WebSocketFrame frame = parseFrame(data);
                    
                    if (frame.opcode == OPCODE_TEXT) {
                        processMessage(conn, frame.payload);
                    } else if (frame.opcode == OPCODE_CLOSE) {
                        break;
                    } else if (frame.opcode == OPCODE_PING) {
                        sendFrame(conn, "", OPCODE_PONG);
                    }
                }
            }
            return;
        }
        
        // Handle CORS preflight requests
        if (method == "OPTIONS") {
            handleCorsPreflight(conn);
            return;
        }
        
        // Handle different endpoints
        if (method == "POST" && path == "/api/auth/register") {
            handleRegister(conn, request);
        } else if (method == "POST" && path == "/api/auth/login") {
            handleLogin(conn, request);
        } else if (method == "GET" && path == "/") {
            // Serve a simple status page
            std::string response = "HTTP/1.1 200 OK\r\n";
            response += "Content-Type: text/plain\r\n";
            response += "Content-Length: 13\r\n";
            response += "Connection: close\r\n";
            response += "\r\n";
            response += "Hello, World!";
            send(conn->socket, response.c_str(), response.length(), 0);
            ::close(conn->socket);
            return;
        } else {
            // Route to server's HTTP handlers
            handleHttpRequest(conn, method, path, request);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Exception in handleClient: " << e.what() << std::endl;
        sendErrorResponse(conn, 500, "Internal Server Error");
    } catch (...) {
        std::cerr << "Unknown exception in handleClient" << std::endl;
        sendErrorResponse(conn, 500, "Internal Server Error");
    }
}

void WebSocketHandler::handleCorsPreflight(std::shared_ptr<WebSocketConnection> conn) {
    std::string response = "HTTP/1.1 200 OK\r\n";
    response += "Access-Control-Allow-Origin: *\r\n";
    response += "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n";
    response += "Access-Control-Allow-Headers: Content-Type, Authorization\r\n";
    response += "Access-Control-Max-Age: 86400\r\n";
    response += "Content-Length: 0\r\n";
    response += "Connection: close\r\n";
    response += "\r\n";
    
    send(conn->socket, response.c_str(), response.length(), 0);
    ::close(conn->socket);
}

void WebSocketHandler::handleRegister(std::shared_ptr<WebSocketConnection> conn, const std::string& request) {
    try {
        // Extract JSON body from request
        size_t bodyStart = request.find("\r\n\r\n");
        if (bodyStart == std::string::npos) {
            sendErrorResponse(conn, 400, "Bad Request");
            return;
        }
        
        std::string body = request.substr(bodyStart + 4);
        
        // Simple JSON parsing (in production, use a proper JSON library)
        // Expected format: {"username": "user@cockpit.com", "email": "user@cockpit.com", "password": "password"}
        
        // For now, just return a success response
        std::string response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: application/json\r\n";
        response += "Access-Control-Allow-Origin: *\r\n";
        response += "Access-Control-Allow-Methods: POST, OPTIONS\r\n";
        response += "Access-Control-Allow-Headers: Content-Type\r\n";
        response += "Connection: close\r\n";
        response += "\r\n";
        response += "{\"success\": true, \"message\": \"User registered successfully\", \"token\": \"demo-token-123\"}";
        
        send(conn->socket, response.c_str(), response.length(), 0);
        ::close(conn->socket);
    } catch (const std::exception& e) {
        std::cerr << "Exception in handleRegister: " << e.what() << std::endl;
        sendErrorResponse(conn, 500, "Internal Server Error");
    }
}

void WebSocketHandler::handleLogin(std::shared_ptr<WebSocketConnection> conn, const std::string& request) {
    try {
        // Extract JSON body from request
        size_t bodyStart = request.find("\r\n\r\n");
        if (bodyStart == std::string::npos) {
            sendErrorResponse(conn, 400, "Bad Request");
            return;
        }
        
        std::string body = request.substr(bodyStart + 4);
        
        // For now, just return a success response
        std::string response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: application/json\r\n";
        response += "Access-Control-Allow-Origin: *\r\n";
        response += "Access-Control-Allow-Methods: POST, OPTIONS\r\n";
        response += "Access-Control-Allow-Headers: Content-Type\r\n";
        response += "Connection: close\r\n";
        response += "\r\n";
        response += "{\"success\": true, \"message\": \"Login successful\", \"token\": \"demo-token-123\", \"user\": {\"id\": 1, \"username\": \"demo@cockpit.com\", \"email\": \"demo@cockpit.com\"}}";
        
        send(conn->socket, response.c_str(), response.length(), 0);
        ::close(conn->socket);
    } catch (const std::exception& e) {
        std::cerr << "Exception in handleLogin: " << e.what() << std::endl;
        sendErrorResponse(conn, 500, "Internal Server Error");
    }
}

void WebSocketHandler::sendErrorResponse(std::shared_ptr<WebSocketConnection> conn, int statusCode, const std::string& message) {
    try {
        std::string response = "HTTP/1.1 " + std::to_string(statusCode) + " " + message + "\r\n";
        response += "Content-Type: application/json\r\n";
        response += "Access-Control-Allow-Origin: *\r\n";
        response += "Connection: close\r\n";
        response += "\r\n";
        response += "{\"success\": false, \"error\": \"" + message + "\"}";
        
        send(conn->socket, response.c_str(), response.length(), 0);
        ::close(conn->socket);
    } catch (const std::exception& e) {
        std::cerr << "Exception in sendErrorResponse: " << e.what() << std::endl;
        // Last resort - just close the socket
        if (conn && conn->socket > 0) {
            ::close(conn->socket);
        }
    }
}

bool WebSocketHandler::performHandshake(int socket, const std::string& request) {
    try {
        // Extract WebSocket key
        std::string key;
        size_t keyStart = request.find("Sec-WebSocket-Key: ");
        if (keyStart != std::string::npos) {
            keyStart += 19;
            size_t keyEnd = request.find("\r\n", keyStart);
            if (keyEnd != std::string::npos) {
                key = request.substr(keyStart, keyEnd - keyStart);
            }
        }
        
        if (key.empty()) {
            return false;
        }
        
        // Generate response
        std::string response = createHandshakeResponse(key);
        return send(socket, response.c_str(), response.length(), 0) > 0;
    } catch (const std::exception& e) {
        std::cerr << "Exception in performHandshake: " << e.what() << std::endl;
        return false;
    }
}

std::string WebSocketHandler::createHandshakeResponse(const std::string& key) {
    std::string magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string concatenated = key + magic;
    std::string sha1Hash = sha1Base64(concatenated);
    
    std::string response = "HTTP/1.1 101 Switching Protocols\r\n";
    response += "Upgrade: websocket\r\n";
    response += "Connection: Upgrade\r\n";
    response += "Sec-WebSocket-Accept: " + sha1Hash + "\r\n";
    response += "\r\n";
    
    return response;
}

WebSocketFrame WebSocketHandler::parseFrame(const std::string& data) {
    WebSocketFrame frame;
    
    if (data.length() < 2) {
        return frame;
    }
    
    uint8_t firstByte = data[0];
    uint8_t secondByte = data[1];
    
    frame.fin = (firstByte & 0x80) != 0;
    frame.opcode = firstByte & 0x0F;
    frame.masked = (secondByte & 0x80) != 0;
    
    uint64_t payloadLength = secondByte & 0x7F;
    size_t headerLength = 2;
    
    if (payloadLength == 126) {
        if (data.length() < 4) return frame;
        payloadLength = (data[2] << 8) | data[3];
        headerLength = 4;
    } else if (payloadLength == 127) {
        if (data.length() < 10) return frame;
        payloadLength = 0;
        for (int i = 0; i < 8; i++) {
            payloadLength = (payloadLength << 8) | data[2 + i];
        }
        headerLength = 10;
    }
    
    if (frame.masked) {
        if (data.length() < headerLength + 4) return frame;
        frame.masking_key = data.substr(headerLength, 4);
        headerLength += 4;
    }
    
    if (data.length() < headerLength + payloadLength) return frame;
    
    frame.payload = data.substr(headerLength, payloadLength);
    
    if (frame.masked) {
        frame.payload = maskData(frame.payload, frame.masking_key);
    }
    
    return frame;
}

std::string WebSocketHandler::createFrame(const std::string& payload, uint8_t opcode) {
    std::string frame;
    
    // First byte: FIN + opcode
    frame.push_back(0x80 | opcode);
    
    // Second byte: MASK + payload length
    if (payload.length() < 126) {
        frame.push_back(payload.length());
    } else if (payload.length() < 65536) {
        frame.push_back(126);
        frame.push_back((payload.length() >> 8) & 0xFF);
        frame.push_back(payload.length() & 0xFF);
    } else {
        frame.push_back(127);
        for (int i = 7; i >= 0; i--) {
            frame.push_back((payload.length() >> (i * 8)) & 0xFF);
        }
    }
    
    frame += payload;
    return frame;
}

std::string WebSocketHandler::maskData(const std::string& data, const std::string& key) {
    std::string masked;
    for (size_t i = 0; i < data.length(); i++) {
        masked.push_back(data[i] ^ key[i % 4]);
    }
    return masked;
}

void WebSocketHandler::processMessage(std::shared_ptr<WebSocketConnection> conn, const std::string& message) {
    // TODO: Parse JSON message and handle different message types
    std::cout << "Received message from " << conn->remote_address << ": " << message << std::endl;
    
    // Echo back for now
    sendFrame(conn, "Echo: " + message, OPCODE_TEXT);
}

void WebSocketHandler::sendFrame(std::shared_ptr<WebSocketConnection> conn, const std::string& payload, uint8_t opcode) {
    if (!conn || !conn->active) {
        return;
    }
    
    try {
        std::string frame = createFrame(payload, opcode);
        send(conn->socket, frame.c_str(), frame.length(), 0);
    } catch (const std::exception& e) {
        std::cerr << "Exception in sendFrame: " << e.what() << std::endl;
    }
}

void WebSocketHandler::closeConnection(std::shared_ptr<WebSocketConnection> conn, uint16_t code) {
    if (!conn) {
        return;
    }
    
    try {
        conn->active = false;
        
        // Send close frame if socket is still valid
        if (conn->socket > 0) {
            std::string closePayload;
            closePayload.push_back((code >> 8) & 0xFF);
            closePayload.push_back(code & 0xFF);
            sendFrame(conn, closePayload, OPCODE_CLOSE);
            
            ::close(conn->socket);
        }
        
        // Remove from connections using socket as key
        std::lock_guard<std::mutex> lock(connectionsMutex_);
        connections_.erase(conn->socket);
    } catch (const std::exception& e) {
        std::cerr << "Exception in closeConnection: " << e.what() << std::endl;
    }
}

void WebSocketHandler::addConnection(int userId, std::shared_ptr<WebSocketConnection> conn) {
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    connections_[userId] = conn;
}

void WebSocketHandler::removeConnection(int userId) {
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    connections_.erase(userId);
}

std::shared_ptr<WebSocketConnection> WebSocketHandler::getConnection(int userId) {
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    auto it = connections_.find(userId);
    return (it != connections_.end()) ? it->second : nullptr;
}

void WebSocketHandler::broadcastMessage(const std::string& message, const std::set<int>& userIds) {
    std::lock_guard<std::mutex> lock(connectionsMutex_);
    for (int userId : userIds) {
        auto it = connections_.find(userId);
        if (it != connections_.end() && it->second && it->second->active) {
            sendFrame(it->second, message, OPCODE_TEXT);
        }
    }
}

void WebSocketHandler::sendToUser(int userId, const std::string& message) {
    auto conn = getConnection(userId);
    if (conn && conn->active) {
        sendFrame(conn, message, OPCODE_TEXT);
    }
}

void WebSocketHandler::disconnectUser(int userId) {
    auto conn = getConnection(userId);
    if (conn) {
        closeConnection(conn);
    }
}

std::string WebSocketHandler::generateWebSocketKey() {
    const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string key;
    for (int i = 0; i < 16; i++) {
        key += chars[rand() % chars.length()];
    }
    return base64Encode(key);
}

std::string WebSocketHandler::sha1Base64(const std::string& input) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(input.c_str()), input.length(), hash);
    
    BIO* bio = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);
    
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, hash, SHA_DIGEST_LENGTH);
    BIO_flush(bio);
    
    BUF_MEM* bufferPtr;
    BIO_get_mem_ptr(bio, &bufferPtr);
    
    std::string result(bufferPtr->data, bufferPtr->length);
    
    BIO_free_all(bio);
    return result;
}

std::string WebSocketHandler::base64Encode(const std::string& data) {
    BIO* bio = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);
    
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, data.c_str(), data.length());
    BIO_flush(bio);
    
    BUF_MEM* bufferPtr;
    BIO_get_mem_ptr(bio, &bufferPtr);
    
    std::string result(bufferPtr->data, bufferPtr->length);
    
    BIO_free_all(bio);
    return result;
}

uint16_t WebSocketHandler::htons(uint16_t hostshort) {
    return ::htons(hostshort);
}

void WebSocketHandler::handleHttpRequest(std::shared_ptr<WebSocketConnection> conn, const std::string& method, const std::string& path, const std::string& request) {
    if (!server_) {
        // If no server reference, return 404
        sendErrorResponse(conn, 404, "Not Found");
        return;
    }
    
    try {
        // Extract headers and body from request
        std::map<std::string, std::string> headers;
        std::string body;
        
        size_t headerEnd = request.find("\r\n\r\n");
        if (headerEnd != std::string::npos) {
            std::string headerSection = request.substr(0, headerEnd);
            body = request.substr(headerEnd + 4);
            
            // Parse headers
            std::istringstream headerStream(headerSection);
            std::string line;
            bool firstLine = true;
            while (std::getline(headerStream, line)) {
                if (firstLine) {
                    firstLine = false;
                    continue; // Skip the request line
                }
                
                if (line.empty() || line == "\r") continue;
                
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
        }
        
        // Call server's route handler
        std::string response;
        server_->handleRequest(method + " " + path + " HTTP/1.1\r\n" + 
                              [&headers]() {
                                  std::string headerStr;
                                  for (const auto& [key, value] : headers) {
                                      headerStr += key + ": " + value + "\r\n";
                                  }
                                  return headerStr;
                              }() + "\r\n" + body, response);
        
        // Send the response
        send(conn->socket, response.c_str(), response.length(), 0);
        ::close(conn->socket);
        
    } catch (const std::exception& e) {
        std::cerr << "Exception in handleHttpRequest: " << e.what() << std::endl;
        sendErrorResponse(conn, 500, "Internal Server Error");
    }
} 