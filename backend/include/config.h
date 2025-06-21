#ifndef CONFIG_H
#define CONFIG_H

#include <string>

// Gmail OAuth2 Configuration
struct GmailConfig {
    static const std::string CLIENT_ID;
    static const std::string CLIENT_SECRET;
    static const std::string REDIRECT_URI;
    static const std::string AUTH_URL;
    static const std::string TOKEN_URL;
    static const std::string SCOPE;
};

// WhatsApp Web Configuration
struct WhatsAppConfig {
    static const std::string API_BASE_URL;
    static const std::string SESSION_ENDPOINT;
};

// Telegram Bot Configuration
struct TelegramConfig {
    static const std::string API_BASE_URL;
    static const std::string BOT_API_URL;
};

// General API Configuration
struct APIConfig {
    static const int REQUEST_TIMEOUT;
    static const int MAX_RETRIES;
    static const std::string USER_AGENT;
};

std::string getDatabasePath();

#endif // CONFIG_H 