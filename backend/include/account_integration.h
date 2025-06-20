#ifndef ACCOUNT_INTEGRATION_H
#define ACCOUNT_INTEGRATION_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <thread>

enum class AccountType {
    EMAIL,
    MESSENGER
};

enum class ProviderType {
    GMAIL,
    OUTLOOK,
    YAHOO_MAIL,
    PROTONMAIL,
    WHATSAPP,
    TELEGRAM,
    FACEBOOK_MESSENGER,
    TWITTER_DM,
    INSTAGRAM_DM
};

struct AccountCredentials {
    std::string id;
    std::string userId;
    AccountType type;
    ProviderType provider;
    std::string email;
    std::string username;
    std::string password;
    std::string accessToken;
    std::string refreshToken;
    std::chrono::system_clock::time_point tokenExpiry;
    bool isActive;
    std::chrono::system_clock::time_point lastSync;
    std::chrono::system_clock::time_point createdAt;
};

struct UnifiedMessage {
    std::string id;
    std::string accountId;
    std::string sender;
    std::string recipient;
    std::string subject;
    std::string content;
    std::string messageType; // "email", "message", "notification"
    std::chrono::system_clock::time_point timestamp;
    bool isRead;
    bool isImportant;
    std::vector<std::string> attachments;
    std::map<std::string, std::string> metadata;
};

class AccountIntegrationManager {
public:
    AccountIntegrationManager();
    ~AccountIntegrationManager();

    // Account Management
    bool addAccount(const std::string& userId, const AccountCredentials& credentials);
    bool removeAccount(const std::string& userId, const std::string& accountId);
    bool updateAccount(const std::string& userId, const std::string& accountId, const AccountCredentials& credentials);
    std::vector<AccountCredentials> getUserAccounts(const std::string& userId);
    bool toggleAccountStatus(const std::string& userId, const std::string& accountId, bool isActive);

    // Message Fetching
    std::vector<UnifiedMessage> fetchNewMessages(const std::string& userId);
    std::vector<UnifiedMessage> fetchMessagesByAccount(const std::string& userId, const std::string& accountId);
    std::vector<UnifiedMessage> searchMessages(const std::string& userId, const std::string& query);

    // Provider-specific methods
    bool connectGmail(const std::string& userId, const std::string& email, const std::string& password);
    bool connectOutlook(const std::string& userId, const std::string& email, const std::string& password);
    bool connectWhatsApp(const std::string& userId, const std::string& phoneNumber, const std::string& password);
    bool connectTelegram(const std::string& userId, const std::string& phoneNumber, const std::string& code);

    // Message Actions
    bool markMessageAsRead(const std::string& userId, const std::string& messageId);
    bool markMessageAsImportant(const std::string& userId, const std::string& messageId);
    bool deleteMessage(const std::string& userId, const std::string& messageId);
    bool replyToMessage(const std::string& userId, const std::string& messageId, const std::string& replyContent);

    // Sync Management
    void startSyncService();
    void stopSyncService();
    bool syncAccount(const std::string& userId, const std::string& accountId);

private:
    // Database operations
    bool saveAccountToDatabase(const AccountCredentials& account);
    bool loadAccountsFromDatabase(const std::string& userId, std::vector<AccountCredentials>& accounts);
    bool saveMessageToDatabase(const UnifiedMessage& message);
    bool loadMessagesFromDatabase(const std::string& userId, std::vector<UnifiedMessage>& messages);

    // Provider-specific implementations
    std::vector<UnifiedMessage> fetchGmailMessages(const AccountCredentials& account);
    std::vector<UnifiedMessage> fetchOutlookMessages(const AccountCredentials& account);
    std::vector<UnifiedMessage> fetchWhatsAppMessages(const AccountCredentials& account);
    std::vector<UnifiedMessage> fetchTelegramMessages(const AccountCredentials& account);
    
    // Connection testing
    bool testGmailConnection(const std::string& email, const std::string& password);
    
    // Helper methods
    std::string generateAccountId();
    std::string generateMessageId();
    bool validateCredentials(const AccountCredentials& credentials);
    std::chrono::system_clock::time_point getCurrentTime();

    // Member variables
    std::map<std::string, AccountCredentials> activeAccounts;
    std::map<std::string, std::vector<UnifiedMessage>> messageCache;
    bool syncServiceRunning;
    std::thread syncThread;
};

#endif // ACCOUNT_INTEGRATION_H 