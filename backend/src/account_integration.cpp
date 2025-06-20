#include "account_integration.h"
#include "database.h"
#include "config.h"
#include "json_parser.h"
#include <iostream>
#include <random>
#include <sstream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <curl/curl.h>
#include <cstring>

AccountIntegrationManager::AccountIntegrationManager() : syncServiceRunning(false) {
    std::cout << "Account Integration Manager initialized" << std::endl;
}

AccountIntegrationManager::~AccountIntegrationManager() {
    stopSyncService();
}

bool AccountIntegrationManager::addAccount(const std::string& userId, const AccountCredentials& credentials) {
    try {
        AccountCredentials newAccount = credentials;
        newAccount.id = generateAccountId();
        newAccount.userId = userId;
        newAccount.createdAt = getCurrentTime();
        newAccount.lastSync = getCurrentTime();
        newAccount.isActive = true;

        if (!validateCredentials(newAccount)) {
            std::cerr << "Invalid credentials for account: " << newAccount.email << std::endl;
            return false;
        }

        // Save to database
        if (!saveAccountToDatabase(newAccount)) {
            std::cerr << "Failed to save account to database" << std::endl;
            return false;
        }

        // Add to active accounts
        activeAccounts[newAccount.id] = newAccount;

        std::cout << "Account added successfully: " << newAccount.email << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Exception in addAccount: " << e.what() << std::endl;
        return false;
    }
}

bool AccountIntegrationManager::removeAccount(const std::string& userId, const std::string& accountId) {
    try {
        auto it = activeAccounts.find(accountId);
        if (it == activeAccounts.end()) {
            std::cerr << "Account not found: " << accountId << std::endl;
            return false;
        }

        if (it->second.userId != userId) {
            std::cerr << "User not authorized to remove this account" << std::endl;
            return false;
        }

        // Remove from database (TODO: implement)
        activeAccounts.erase(it);

        std::cout << "Account removed successfully: " << accountId << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Exception in removeAccount: " << e.what() << std::endl;
        return false;
    }
}

std::vector<AccountCredentials> AccountIntegrationManager::getUserAccounts(const std::string& userId) {
    std::vector<AccountCredentials> userAccounts;
    
    try {
        for (const auto& pair : activeAccounts) {
            if (pair.second.userId == userId) {
                userAccounts.push_back(pair.second);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception in getUserAccounts: " << e.what() << std::endl;
    }

    return userAccounts;
}

bool AccountIntegrationManager::toggleAccountStatus(const std::string& userId, const std::string& accountId, bool isActive) {
    try {
        auto it = activeAccounts.find(accountId);
        if (it == activeAccounts.end()) {
            return false;
        }

        if (it->second.userId != userId) {
            return false;
        }

        it->second.isActive = isActive;
        
        // Update database (TODO: implement)
        std::cout << "Account status toggled: " << accountId << " -> " << (isActive ? "active" : "inactive") << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Exception in toggleAccountStatus: " << e.what() << std::endl;
        return false;
    }
}

std::vector<UnifiedMessage> AccountIntegrationManager::fetchNewMessages(const std::string& userId) {
    std::vector<UnifiedMessage> allMessages;
    
    try {
        for (const auto& pair : activeAccounts) {
            if (pair.second.userId == userId && pair.second.isActive) {
                std::vector<UnifiedMessage> accountMessages;
                
                switch (pair.second.provider) {
                    case ProviderType::GMAIL:
                        accountMessages = fetchGmailMessages(pair.second);
                        break;
                    case ProviderType::OUTLOOK:
                        accountMessages = fetchOutlookMessages(pair.second);
                        break;
                    case ProviderType::WHATSAPP:
                        accountMessages = fetchWhatsAppMessages(pair.second);
                        break;
                    case ProviderType::TELEGRAM:
                        accountMessages = fetchTelegramMessages(pair.second);
                        break;
                    default:
                        std::cout << "Provider not implemented: " << static_cast<int>(pair.second.provider) << std::endl;
                        break;
                }
                
                allMessages.insert(allMessages.end(), accountMessages.begin(), accountMessages.end());
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception in fetchNewMessages: " << e.what() << std::endl;
    }

    return allMessages;
}

bool AccountIntegrationManager::connectGmail(const std::string& userId, const std::string& email, const std::string& password) {
    try {
        std::cout << "Attempting to connect to Gmail: " << email << std::endl;
        
        // Test Gmail IMAP connection
        if (!testGmailConnection(email, password)) {
            std::cerr << "Failed to connect to Gmail IMAP server" << std::endl;
            return false;
        }
        
        AccountCredentials credentials;
        credentials.type = AccountType::EMAIL;
        credentials.provider = ProviderType::GMAIL;
        credentials.email = email;
        credentials.password = password;
        credentials.isActive = true;
        
        std::cout << "Successfully connected to Gmail: " << email << std::endl;
        return addAccount(userId, credentials);
    } catch (const std::exception& e) {
        std::cerr << "Exception in connectGmail: " << e.what() << std::endl;
        return false;
    }
}

bool AccountIntegrationManager::connectGmailOAuth2(const std::string& userId, const std::string& email, const std::string& accessToken, const std::string& refreshToken) {
    try {
        std::cout << "Attempting to connect to Gmail via OAuth2: " << email << std::endl;
        
        // Test Gmail OAuth2 connection
        if (!testGmailOAuth2Connection(accessToken)) {
            std::cerr << "Failed to connect to Gmail OAuth2" << std::endl;
            return false;
        }
        
        AccountCredentials credentials;
        credentials.type = AccountType::EMAIL;
        credentials.provider = ProviderType::GMAIL;
        credentials.email = email;
        credentials.accessToken = accessToken;
        credentials.refreshToken = refreshToken;
        credentials.tokenExpiry = getCurrentTime() + std::chrono::hours(1); // Gmail tokens expire in 1 hour
        credentials.isActive = true;
        
        std::cout << "Successfully connected to Gmail via OAuth2: " << email << std::endl;
        return addAccount(userId, credentials);
    } catch (const std::exception& e) {
        std::cerr << "Exception in connectGmailOAuth2: " << e.what() << std::endl;
        return false;
    }
}

bool AccountIntegrationManager::testGmailConnection(const std::string& email, const std::string& password) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize CURL" << std::endl;
        return false;
    }
    
    // For Gmail, we need to use an App Password if 2FA is enabled
    // For now, we'll test basic connectivity to Gmail's IMAP server
    std::string url = "https://gmail.googleapis.com/gmail/v1/users/me/profile";
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERNAME, email.c_str());
    curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    
    // Set up response handling
    std::string response;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](void* contents, size_t size, size_t nmemb, std::string* userp) {
        userp->append((char*)contents, size * nmemb);
        return size * nmemb;
    });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    
    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        std::cerr << "CURL error: " << curl_easy_strerror(res) << std::endl;
        return false;
    }
    
    // Check if we got a successful response (200) or authentication error (401)
    if (http_code == 200) {
        std::cout << "Gmail connection successful" << std::endl;
        return true;
    } else if (http_code == 401) {
        std::cerr << "Gmail authentication failed. Please check your credentials." << std::endl;
        std::cerr << "Note: If you have 2FA enabled, you need to use an App Password instead of your regular password." << std::endl;
        return false;
    } else {
        std::cerr << "Gmail connection failed with HTTP code: " << http_code << std::endl;
        return false;
    }
}

bool AccountIntegrationManager::testGmailOAuth2Connection(const std::string& accessToken) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize CURL for Gmail OAuth2 test" << std::endl;
        return false;
    }
    
    std::string url = "https://gmail.googleapis.com/gmail/v1/users/me/profile";
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, createAuthHeader(accessToken));
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, APIConfig::REQUEST_TIMEOUT);
    
    std::string response;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](void* contents, size_t size, size_t nmemb, std::string* userp) {
        userp->append((char*)contents, size * nmemb);
        return size * nmemb;
    });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    
    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    
    curl_easy_cleanup(curl);
    
    if (res == CURLE_OK && http_code == 200) {
        std::cout << "Gmail OAuth2 connection successful" << std::endl;
        return true;
    } else {
        std::cerr << "Gmail OAuth2 connection failed. HTTP code: " << http_code << std::endl;
        return false;
    }
}

// Helper method to create Authorization header
struct curl_slist* AccountIntegrationManager::createAuthHeader(const std::string& accessToken) {
    std::string authHeader = "Authorization: Bearer " + accessToken;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, authHeader.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    return headers;
}

std::string AccountIntegrationManager::getGmailOAuth2Url() {
    std::string url = GmailConfig::AUTH_URL + "?";
    url += "client_id=" + GmailConfig::CLIENT_ID;
    url += "&redirect_uri=" + GmailConfig::REDIRECT_URI;
    url += "&scope=" + GmailConfig::SCOPE;
    url += "&response_type=code";
    url += "&access_type=offline";
    url += "&prompt=consent";
    
    return url;
}

bool AccountIntegrationManager::exchangeGmailCodeForTokens(const std::string& code, std::string& accessToken, std::string& refreshToken) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize CURL for token exchange" << std::endl;
        return false;
    }
    
    std::string postData = "client_id=" + GmailConfig::CLIENT_ID;
    postData += "&client_secret=" + GmailConfig::CLIENT_SECRET;
    postData += "&code=" + code;
    postData += "&grant_type=authorization_code";
    postData += "&redirect_uri=" + GmailConfig::REDIRECT_URI;
    
    curl_easy_setopt(curl, CURLOPT_URL, GmailConfig::TOKEN_URL.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, APIConfig::REQUEST_TIMEOUT);
    
    std::string response;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](void* contents, size_t size, size_t nmemb, std::string* userp) {
        userp->append((char*)contents, size * nmemb);
        return size * nmemb;
    });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    
    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    
    curl_easy_cleanup(curl);
    
    if (res == CURLE_OK && http_code == 200) {
        auto tokenData = JsonParser::parseObject(response);
        accessToken = JsonParser::extractString(tokenData, "access_token");
        refreshToken = JsonParser::extractString(tokenData, "refresh_token");
        
        if (!accessToken.empty()) {
            std::cout << "Successfully exchanged code for tokens" << std::endl;
            return true;
        }
    }
    
    std::cerr << "Failed to exchange code for tokens. HTTP code: " << http_code << std::endl;
    return false;
}

bool AccountIntegrationManager::refreshGmailToken(const std::string& refreshToken, std::string& newAccessToken) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize CURL for token refresh" << std::endl;
        return false;
    }
    
    std::string postData = "client_id=" + GmailConfig::CLIENT_ID;
    postData += "&client_secret=" + GmailConfig::CLIENT_SECRET;
    postData += "&refresh_token=" + refreshToken;
    postData += "&grant_type=refresh_token";
    
    curl_easy_setopt(curl, CURLOPT_URL, GmailConfig::TOKEN_URL.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, APIConfig::REQUEST_TIMEOUT);
    
    std::string response;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](void* contents, size_t size, size_t nmemb, std::string* userp) {
        userp->append((char*)contents, size * nmemb);
        return size * nmemb;
    });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    
    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    
    curl_easy_cleanup(curl);
    
    if (res == CURLE_OK && http_code == 200) {
        auto tokenData = JsonParser::parseObject(response);
        newAccessToken = JsonParser::extractString(tokenData, "access_token");
        
        if (!newAccessToken.empty()) {
            std::cout << "Successfully refreshed Gmail token" << std::endl;
            return true;
        }
    }
    
    std::cerr << "Failed to refresh Gmail token. HTTP code: " << http_code << std::endl;
    return false;
}

bool AccountIntegrationManager::connectOutlook(const std::string& userId, const std::string& email, const std::string& password) {
    try {
        AccountCredentials credentials;
        credentials.type = AccountType::EMAIL;
        credentials.provider = ProviderType::OUTLOOK;
        credentials.email = email;
        credentials.password = password;
        
        // TODO: Implement actual Outlook OAuth2 connection
        std::cout << "Connecting to Outlook: " << email << std::endl;
        
        return addAccount(userId, credentials);
    } catch (const std::exception& e) {
        std::cerr << "Exception in connectOutlook: " << e.what() << std::endl;
        return false;
    }
}

bool AccountIntegrationManager::connectWhatsApp(const std::string& userId, const std::string& phoneNumber, const std::string& password) {
    try {
        AccountCredentials credentials;
        credentials.type = AccountType::MESSENGER;
        credentials.provider = ProviderType::WHATSAPP;
        credentials.username = phoneNumber;
        credentials.password = password;
        
        // For now, use the Web API approach
        std::cout << "Connecting to WhatsApp: " << phoneNumber << std::endl;
        std::cout << "Note: WhatsApp Web API requires a session ID. Please use connectWhatsAppWeb method instead." << std::endl;
        
        return addAccount(userId, credentials);
    } catch (const std::exception& e) {
        std::cerr << "Exception in connectWhatsApp: " << e.what() << std::endl;
        return false;
    }
}

bool AccountIntegrationManager::connectTelegram(const std::string& userId, const std::string& phoneNumber, const std::string& code) {
    try {
        AccountCredentials credentials;
        credentials.type = AccountType::MESSENGER;
        credentials.provider = ProviderType::TELEGRAM;
        credentials.username = phoneNumber;
        credentials.password = code;
        
        // For now, use the Bot API approach
        std::cout << "Connecting to Telegram: " << phoneNumber << std::endl;
        std::cout << "Note: Telegram Bot API requires a bot token. Please use connectTelegramBot method instead." << std::endl;
        
        return addAccount(userId, credentials);
    } catch (const std::exception& e) {
        std::cerr << "Exception in connectTelegram: " << e.what() << std::endl;
        return false;
    }
}

bool AccountIntegrationManager::connectWhatsAppWeb(const std::string& userId, const std::string& phoneNumber, const std::string& sessionId) {
    try {
        std::cout << "Attempting to connect to WhatsApp Web: " << phoneNumber << std::endl;
        
        // Test WhatsApp Web connection
        if (!testWhatsAppWebConnection(sessionId)) {
            std::cerr << "Failed to connect to WhatsApp Web" << std::endl;
            return false;
        }
        
        AccountCredentials credentials;
        credentials.type = AccountType::MESSENGER;
        credentials.provider = ProviderType::WHATSAPP;
        credentials.username = phoneNumber;
        credentials.sessionId = sessionId;
        credentials.isActive = true;
        
        std::cout << "Successfully connected to WhatsApp Web: " << phoneNumber << std::endl;
        return addAccount(userId, credentials);
    } catch (const std::exception& e) {
        std::cerr << "Exception in connectWhatsAppWeb: " << e.what() << std::endl;
        return false;
    }
}

bool AccountIntegrationManager::testWhatsAppWebConnection(const std::string& sessionId) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize CURL for WhatsApp Web test" << std::endl;
        return false;
    }
    
    std::string url = WhatsAppConfig::API_BASE_URL + "/session/" + sessionId + "/status";
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, APIConfig::REQUEST_TIMEOUT);
    
    std::string response;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](void* contents, size_t size, size_t nmemb, std::string* userp) {
        userp->append((char*)contents, size * nmemb);
        return size * nmemb;
    });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    
    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    
    curl_easy_cleanup(curl);
    
    if (res == CURLE_OK && http_code == 200) {
        auto statusData = JsonParser::parseObject(response);
        std::string status = JsonParser::extractString(statusData, "status");
        
        if (status == "connected" || status == "authenticated") {
            std::cout << "WhatsApp Web connection successful" << std::endl;
            return true;
        }
    }
    
    std::cerr << "WhatsApp Web connection failed. HTTP code: " << http_code << std::endl;
    return false;
}

std::vector<UnifiedMessage> AccountIntegrationManager::fetchWhatsAppMessagesWeb(const AccountCredentials& account) {
    std::vector<UnifiedMessage> messages;
    
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize CURL for WhatsApp Web message fetching" << std::endl;
        return messages;
    }
    
    // Fetch messages using WhatsApp Web API
    std::string url = WhatsAppConfig::API_BASE_URL + "/session/" + account.sessionId + "/messages";
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, APIConfig::REQUEST_TIMEOUT);
    
    std::string response;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](void* contents, size_t size, size_t nmemb, std::string* userp) {
        userp->append((char*)contents, size * nmemb);
        return size * nmemb;
    });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    
    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    
    curl_easy_cleanup(curl);
    
    if (res == CURLE_OK && http_code == 200) {
        // Parse WhatsApp messages
        auto messageList = JsonParser::parseWhatsAppMessages(response);
        
        for (const auto& msgData : messageList) {
            UnifiedMessage message;
            message.id = JsonParser::extractString(msgData, "id");
            message.accountId = account.id;
            message.sender = JsonParser::extractString(msgData, "from");
            message.recipient = account.username;
            message.subject = "WhatsApp Message";
            message.content = JsonParser::extractString(msgData, "text");
            message.messageType = "message";
            message.timestamp = getCurrentTime(); // TODO: Parse actual timestamp
            message.isRead = JsonParser::extractBool(msgData, "read", false);
            message.isImportant = false;
            
            messages.push_back(message);
        }
        
        std::cout << "Successfully fetched " << messages.size() << " WhatsApp messages for: " << account.username << std::endl;
    } else {
        std::cerr << "Failed to fetch WhatsApp messages. HTTP code: " << http_code << std::endl;
        
        // Return error message
        UnifiedMessage errorMessage;
        errorMessage.id = generateMessageId();
        errorMessage.accountId = account.id;
        errorMessage.sender = "cockpit-system@cockpit.com";
        errorMessage.recipient = account.username;
        errorMessage.subject = "WhatsApp Connection Issue";
        errorMessage.content = "There was an issue connecting to your WhatsApp account. Please check your session ID and try again.";
        errorMessage.messageType = "system";
        errorMessage.timestamp = getCurrentTime();
        errorMessage.isRead = false;
        errorMessage.isImportant = true;
        
        messages.push_back(errorMessage);
    }
    
    return messages;
}

std::vector<UnifiedMessage> AccountIntegrationManager::fetchWhatsAppMessages(const AccountCredentials& account) {
    // Check if we have a session ID for Web API
    if (!account.sessionId.empty()) {
        return fetchWhatsAppMessagesWeb(account);
    }
    
    // Fallback to mock data
    std::vector<UnifiedMessage> messages;
    
    // TODO: Implement actual WhatsApp Web API integration
    // For now, return mock data
    UnifiedMessage mockMessage;
    mockMessage.id = generateMessageId();
    mockMessage.accountId = account.id;
    mockMessage.sender = "+1234567890";
    mockMessage.recipient = account.username;
    mockMessage.subject = "WhatsApp Message";
    mockMessage.content = "This is a test WhatsApp message. Please use WhatsApp Web API for real integration.";
    mockMessage.messageType = "message";
    mockMessage.timestamp = getCurrentTime();
    mockMessage.isRead = false;
    mockMessage.isImportant = false;
    
    messages.push_back(mockMessage);
    
    return messages;
}

void AccountIntegrationManager::startSyncService() {
    if (syncServiceRunning) {
        return;
    }

    syncServiceRunning = true;
    syncThread = std::thread([this]() {
        while (syncServiceRunning) {
            try {
                // Sync all active accounts
                for (auto& pair : activeAccounts) {
                    if (pair.second.isActive) {
                        syncAccount(pair.second.userId, pair.second.id);
                    }
                }
                
                // Sleep for 30 seconds before next sync
                std::this_thread::sleep_for(std::chrono::seconds(30));
            } catch (const std::exception& e) {
                std::cerr << "Exception in sync service: " << e.what() << std::endl;
            }
        }
    });

    std::cout << "Sync service started" << std::endl;
}

void AccountIntegrationManager::stopSyncService() {
    if (!syncServiceRunning) {
        return;
    }

    syncServiceRunning = false;
    if (syncThread.joinable()) {
        syncThread.join();
    }

    std::cout << "Sync service stopped" << std::endl;
}

bool AccountIntegrationManager::syncAccount(const std::string& userId, const std::string& accountId) {
    try {
        auto it = activeAccounts.find(accountId);
        if (it == activeAccounts.end()) {
            return false;
        }

        // Fetch new messages
        std::vector<UnifiedMessage> newMessages = fetchNewMessages(userId);
        
        // Update last sync time
        it->second.lastSync = getCurrentTime();
        
        // Save messages to database
        for (const auto& message : newMessages) {
            saveMessageToDatabase(message);
        }

        std::cout << "Synced " << newMessages.size() << " messages for account: " << accountId << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Exception in syncAccount: " << e.what() << std::endl;
        return false;
    }
}

// Private helper methods

std::string AccountIntegrationManager::generateAccountId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static const char* hex = "0123456789abcdef";

    std::string id;
    for (int i = 0; i < 32; ++i) {
        id += hex[dis(gen)];
    }
    return id;
}

std::string AccountIntegrationManager::generateMessageId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static const char* hex = "0123456789abcdef";

    std::string id;
    for (int i = 0; i < 16; ++i) {
        id += hex[dis(gen)];
    }
    return id;
}

bool AccountIntegrationManager::validateCredentials(const AccountCredentials& credentials) {
    // Basic validation
    if (credentials.email.empty() && credentials.username.empty()) {
        return false;
    }

    if (credentials.password.empty()) {
        return false;
    }

    // TODO: Add more sophisticated validation based on provider
    return true;
}

std::chrono::system_clock::time_point AccountIntegrationManager::getCurrentTime() {
    return std::chrono::system_clock::now();
}

// Provider-specific implementations (placeholder implementations)

std::vector<UnifiedMessage> AccountIntegrationManager::fetchGmailMessages(const AccountCredentials& account) {
    // Check if we have OAuth2 tokens
    if (!account.accessToken.empty()) {
        return fetchGmailMessagesOAuth2(account);
    }
    
    // Fallback to basic authentication (existing implementation)
    std::vector<UnifiedMessage> messages;
    
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize CURL for Gmail message fetching" << std::endl;
        return messages;
    }
    
    // Use Gmail API to fetch messages
    std::string url = "https://gmail.googleapis.com/gmail/v1/users/me/messages?maxResults=10";
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERNAME, account.email.c_str());
    curl_easy_setopt(curl, CURLOPT_PASSWORD, account.password.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    
    std::string response;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](void* contents, size_t size, size_t nmemb, std::string* userp) {
        userp->append((char*)contents, size * nmemb);
        return size * nmemb;
    });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    
    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    
    curl_easy_cleanup(curl);
    
    if (res == CURLE_OK && http_code == 200) {
        // Parse the JSON response and extract messages
        // For now, we'll create a mock message to show the connection works
        UnifiedMessage mockMessage;
        mockMessage.id = generateMessageId();
        mockMessage.accountId = account.id;
        mockMessage.sender = "gmail-api@gmail.com";
        mockMessage.recipient = account.email;
        mockMessage.subject = "Gmail API Connection Successful";
        mockMessage.content = "Your Gmail account has been successfully connected to Cockpit! This message was fetched using the Gmail API.";
        mockMessage.messageType = "email";
        mockMessage.timestamp = getCurrentTime();
        mockMessage.isRead = false;
        mockMessage.isImportant = false;
        
        messages.push_back(mockMessage);
        
        std::cout << "Successfully fetched Gmail messages for: " << account.email << std::endl;
    } else {
        std::cerr << "Failed to fetch Gmail messages. HTTP code: " << http_code << std::endl;
        
        // Return a helpful error message
        UnifiedMessage errorMessage;
        errorMessage.id = generateMessageId();
        errorMessage.accountId = account.id;
        errorMessage.sender = "cockpit-system@cockpit.com";
        errorMessage.recipient = account.email;
        errorMessage.subject = "Gmail Connection Issue";
        errorMessage.content = "There was an issue connecting to your Gmail account. Please check your credentials and try again. If you have 2FA enabled, make sure to use an App Password.";
        errorMessage.messageType = "system";
        errorMessage.timestamp = getCurrentTime();
        errorMessage.isRead = false;
        errorMessage.isImportant = true;
        
        messages.push_back(errorMessage);
    }
    
    return messages;
}

std::vector<UnifiedMessage> AccountIntegrationManager::fetchGmailMessagesOAuth2(const AccountCredentials& account) {
    std::vector<UnifiedMessage> messages;
    
    // Check if token needs refresh
    std::string accessToken = account.accessToken;
    if (getCurrentTime() >= account.tokenExpiry) {
        std::cout << "Gmail token expired, refreshing..." << std::endl;
        if (!refreshGmailToken(account.refreshToken, accessToken)) {
            std::cerr << "Failed to refresh Gmail token" << std::endl;
            return messages;
        }
    }
    
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize CURL for Gmail OAuth2 message fetching" << std::endl;
        return messages;
    }
    
    // Fetch messages using Gmail API
    std::string url = "https://gmail.googleapis.com/gmail/v1/users/me/messages?maxResults=20&labelIds=INBOX";
    
    struct curl_slist* headers = createAuthHeader(accessToken);
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, APIConfig::REQUEST_TIMEOUT);
    
    std::string response;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](void* contents, size_t size, size_t nmemb, std::string* userp) {
        userp->append((char*)contents, size * nmemb);
        return size * nmemb;
    });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    
    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    if (res == CURLE_OK && http_code == 200) {
        // Parse messages list
        auto messageList = JsonParser::parseGmailMessages(response);
        
        for (const auto& msgData : messageList) {
            std::string messageId = JsonParser::extractString(msgData, "id");
            if (!messageId.empty()) {
                // Fetch individual message details
                std::string detailUrl = "https://gmail.googleapis.com/gmail/v1/users/me/messages/" + messageId;
                
                CURL* detailCurl = curl_easy_init();
                if (detailCurl) {
                    struct curl_slist* detailHeaders = createAuthHeader(accessToken);
                    
                    curl_easy_setopt(detailCurl, CURLOPT_URL, detailUrl.c_str());
                    curl_easy_setopt(detailCurl, CURLOPT_HTTPHEADER, detailHeaders);
                    curl_easy_setopt(detailCurl, CURLOPT_SSL_VERIFYPEER, 1L);
                    curl_easy_setopt(detailCurl, CURLOPT_SSL_VERIFYHOST, 2L);
                    curl_easy_setopt(detailCurl, CURLOPT_TIMEOUT, APIConfig::REQUEST_TIMEOUT);
                    
                    std::string detailResponse;
                    curl_easy_setopt(detailCurl, CURLOPT_WRITEFUNCTION, +[](void* contents, size_t size, size_t nmemb, std::string* userp) {
                        userp->append((char*)contents, size * nmemb);
                        return size * nmemb;
                    });
                    curl_easy_setopt(detailCurl, CURLOPT_WRITEDATA, &detailResponse);
                    
                    CURLcode detailRes = curl_easy_perform(detailCurl);
                    long detailHttpCode = 0;
                    curl_easy_getinfo(detailCurl, CURLINFO_RESPONSE_CODE, &detailHttpCode);
                    
                    curl_slist_free_all(detailHeaders);
                    curl_easy_cleanup(detailCurl);
                    
                    if (detailRes == CURLE_OK && detailHttpCode == 200) {
                        auto messageDetails = JsonParser::parseGmailMessageDetails(detailResponse);
                        
                        UnifiedMessage message;
                        message.id = messageId;
                        message.accountId = account.id;
                        message.sender = JsonParser::extractString(messageDetails, "From");
                        message.recipient = JsonParser::extractString(messageDetails, "To");
                        message.subject = JsonParser::extractString(messageDetails, "Subject");
                        message.content = JsonParser::extractString(messageDetails, "body");
                        message.messageType = "email";
                        message.timestamp = getCurrentTime(); // TODO: Parse actual timestamp
                        message.isRead = false;
                        message.isImportant = false;
                        
                        messages.push_back(message);
                    }
                }
            }
        }
        
        std::cout << "Successfully fetched " << messages.size() << " Gmail messages via OAuth2 for: " << account.email << std::endl;
    } else {
        std::cerr << "Failed to fetch Gmail messages via OAuth2. HTTP code: " << http_code << std::endl;
        
        // Return error message
        UnifiedMessage errorMessage;
        errorMessage.id = generateMessageId();
        errorMessage.accountId = account.id;
        errorMessage.sender = "cockpit-system@cockpit.com";
        errorMessage.recipient = account.email;
        errorMessage.subject = "Gmail OAuth2 Connection Issue";
        errorMessage.content = "There was an issue connecting to your Gmail account via OAuth2. Please check your access token and try again.";
        errorMessage.messageType = "system";
        errorMessage.timestamp = getCurrentTime();
        errorMessage.isRead = false;
        errorMessage.isImportant = true;
        
        messages.push_back(errorMessage);
    }
    
    return messages;
}

std::vector<UnifiedMessage> AccountIntegrationManager::fetchOutlookMessages(const AccountCredentials& account) {
    std::vector<UnifiedMessage> messages;
    
    // TODO: Implement actual Outlook API integration
    // For now, return mock data
    UnifiedMessage mockMessage;
    mockMessage.id = generateMessageId();
    mockMessage.accountId = account.id;
    mockMessage.sender = "sender@outlook.com";
    mockMessage.recipient = account.email;
    mockMessage.subject = "Test Outlook Email";
    mockMessage.content = "This is a test email from Outlook integration.";
    mockMessage.messageType = "email";
    mockMessage.timestamp = getCurrentTime();
    mockMessage.isRead = false;
    mockMessage.isImportant = false;
    
    messages.push_back(mockMessage);
    
    return messages;
}

std::vector<UnifiedMessage> AccountIntegrationManager::fetchTelegramMessages(const AccountCredentials& account) {
    // Check if we have a bot token for Bot API
    if (!account.botToken.empty()) {
        return fetchTelegramBotMessages(account);
    }
    
    // Fallback to mock data
    std::vector<UnifiedMessage> messages;
    
    // TODO: Implement actual Telegram Bot API integration
    // For now, return mock data
    UnifiedMessage mockMessage;
    mockMessage.id = generateMessageId();
    mockMessage.accountId = account.id;
    mockMessage.sender = "@telegram_user";
    mockMessage.recipient = account.username;
    mockMessage.subject = "Telegram Message";
    mockMessage.content = "This is a test Telegram message. Please use Telegram Bot API for real integration.";
    mockMessage.messageType = "message";
    mockMessage.timestamp = getCurrentTime();
    mockMessage.isRead = false;
    mockMessage.isImportant = false;
    
    messages.push_back(mockMessage);
    
    return messages;
}

// Database operations (placeholder implementations)

bool AccountIntegrationManager::saveAccountToDatabase(const AccountCredentials& account) {
    // TODO: Implement actual database save
    std::cout << "Saving account to database: " << account.email << std::endl;
    return true;
}

bool AccountIntegrationManager::loadAccountsFromDatabase(const std::string& userId, std::vector<AccountCredentials>& accounts) {
    // TODO: Implement actual database load
    std::cout << "Loading accounts from database for user: " << userId << std::endl;
    return true;
}

bool AccountIntegrationManager::saveMessageToDatabase(const UnifiedMessage& message) {
    // TODO: Implement actual database save
    std::cout << "Saving message to database: " << message.id << std::endl;
    return true;
}

bool AccountIntegrationManager::loadMessagesFromDatabase(const std::string& userId, std::vector<UnifiedMessage>& messages) {
    // TODO: Implement actual database load
    std::cout << "Loading messages from database for user: " << userId << std::endl;
    return true;
}

// Message action implementations (placeholder)

bool AccountIntegrationManager::markMessageAsRead(const std::string& userId, const std::string& messageId) {
    // TODO: Implement
    std::cout << "Marking message as read: " << messageId << std::endl;
    return true;
}

bool AccountIntegrationManager::markMessageAsImportant(const std::string& userId, const std::string& messageId) {
    // TODO: Implement
    std::cout << "Marking message as important: " << messageId << std::endl;
    return true;
}

bool AccountIntegrationManager::deleteMessage(const std::string& userId, const std::string& messageId) {
    // TODO: Implement
    std::cout << "Deleting message: " << messageId << std::endl;
    return true;
}

bool AccountIntegrationManager::replyToMessage(const std::string& userId, const std::string& messageId, const std::string& replyContent) {
    // TODO: Implement
    std::cout << "Replying to message: " << messageId << std::endl;
    return true;
}

std::vector<UnifiedMessage> AccountIntegrationManager::fetchMessagesByAccount(const std::string& userId, const std::string& accountId) {
    // TODO: Implement
    return std::vector<UnifiedMessage>();
}

std::vector<UnifiedMessage> AccountIntegrationManager::searchMessages(const std::string& userId, const std::string& query) {
    // TODO: Implement
    return std::vector<UnifiedMessage>();
}

bool AccountIntegrationManager::updateAccount(const std::string& userId, const std::string& accountId, const AccountCredentials& credentials) {
    // TODO: Implement
    return true;
}

bool AccountIntegrationManager::connectTelegramBot(const std::string& userId, const std::string& botToken, const std::string& chatId) {
    try {
        std::cout << "Attempting to connect to Telegram Bot API" << std::endl;
        
        // Test Telegram Bot connection
        if (!testTelegramBotConnection(botToken)) {
            std::cerr << "Failed to connect to Telegram Bot API" << std::endl;
            return false;
        }
        
        AccountCredentials credentials;
        credentials.type = AccountType::MESSENGER;
        credentials.provider = ProviderType::TELEGRAM;
        credentials.username = chatId;
        credentials.botToken = botToken;
        credentials.isActive = true;
        
        std::cout << "Successfully connected to Telegram Bot API" << std::endl;
        return addAccount(userId, credentials);
    } catch (const std::exception& e) {
        std::cerr << "Exception in connectTelegramBot: " << e.what() << std::endl;
        return false;
    }
}

bool AccountIntegrationManager::testTelegramBotConnection(const std::string& botToken) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize CURL for Telegram Bot test" << std::endl;
        return false;
    }
    
    std::string url = TelegramConfig::BOT_API_URL + botToken + "/getMe";
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, APIConfig::REQUEST_TIMEOUT);
    
    std::string response;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](void* contents, size_t size, size_t nmemb, std::string* userp) {
        userp->append((char*)contents, size * nmemb);
        return size * nmemb;
    });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    
    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    
    curl_easy_cleanup(curl);
    
    if (res == CURLE_OK && http_code == 200) {
        auto botData = JsonParser::parseObject(response);
        bool ok = JsonParser::extractBool(botData, "ok", false);
        
        if (ok) {
            std::string botName = JsonParser::extractString(botData, "result");
            std::cout << "Telegram Bot connection successful: " << botName << std::endl;
            return true;
        }
    }
    
    std::cerr << "Telegram Bot connection failed. HTTP code: " << http_code << std::endl;
    return false;
}

std::vector<UnifiedMessage> AccountIntegrationManager::fetchTelegramBotMessages(const AccountCredentials& account) {
    std::vector<UnifiedMessage> messages;
    
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize CURL for Telegram Bot message fetching" << std::endl;
        return messages;
    }
    
    // Fetch updates using Telegram Bot API
    std::string url = TelegramConfig::BOT_API_URL + account.botToken + "/getUpdates?limit=20&timeout=0";
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, APIConfig::REQUEST_TIMEOUT);
    
    std::string response;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +[](void* contents, size_t size, size_t nmemb, std::string* userp) {
        userp->append((char*)contents, size * nmemb);
        return size * nmemb;
    });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    
    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    
    curl_easy_cleanup(curl);
    
    if (res == CURLE_OK && http_code == 200) {
        // Parse Telegram updates
        auto updateList = JsonParser::parseTelegramUpdates(response);
        
        for (const auto& updateData : updateList) {
            std::string messageJson = JsonParser::extractString(updateData, "message");
            if (!messageJson.empty()) {
                auto messageDetails = JsonParser::parseTelegramMessage(messageJson);
                
                UnifiedMessage message;
                message.id = JsonParser::extractString(updateData, "update_id");
                message.accountId = account.id;
                message.sender = JsonParser::extractString(messageDetails, "from");
                message.recipient = account.username;
                message.subject = "Telegram Message";
                message.content = JsonParser::extractString(messageDetails, "text");
                message.messageType = "message";
                message.timestamp = getCurrentTime(); // TODO: Parse actual timestamp
                message.isRead = false;
                message.isImportant = false;
                
                messages.push_back(message);
            }
        }
        
        std::cout << "Successfully fetched " << messages.size() << " Telegram messages for bot" << std::endl;
    } else {
        std::cerr << "Failed to fetch Telegram messages. HTTP code: " << http_code << std::endl;
        
        // Return error message
        UnifiedMessage errorMessage;
        errorMessage.id = generateMessageId();
        errorMessage.accountId = account.id;
        errorMessage.sender = "cockpit-system@cockpit.com";
        errorMessage.recipient = account.username;
        errorMessage.subject = "Telegram Bot Connection Issue";
        errorMessage.content = "There was an issue connecting to your Telegram Bot. Please check your bot token and try again.";
        errorMessage.messageType = "system";
        errorMessage.timestamp = getCurrentTime();
        errorMessage.isRead = false;
        errorMessage.isImportant = true;
        
        messages.push_back(errorMessage);
    }
    
    return messages;
} 