#include "config.h"

// Gmail OAuth2 Configuration
const std::string GmailConfig::CLIENT_ID = "YOUR_GMAIL_CLIENT_ID";
const std::string GmailConfig::CLIENT_SECRET = "YOUR_GMAIL_CLIENT_SECRET";
const std::string GmailConfig::REDIRECT_URI = "http://localhost:8080/oauth/gmail/callback";
const std::string GmailConfig::AUTH_URL = "https://accounts.google.com/o/oauth2/v2/auth";
const std::string GmailConfig::TOKEN_URL = "https://oauth2.googleapis.com/token";
const std::string GmailConfig::SCOPE = "https://www.googleapis.com/auth/gmail.readonly https://www.googleapis.com/auth/gmail.modify";

// WhatsApp Web Configuration
const std::string WhatsAppConfig::API_BASE_URL = "https://api.whatsapp.com";
const std::string WhatsAppConfig::SESSION_ENDPOINT = "/session";

// Telegram Bot Configuration
const std::string TelegramConfig::API_BASE_URL = "https://api.telegram.org";
const std::string TelegramConfig::BOT_API_URL = "https://api.telegram.org/bot";

// General API Configuration
const int APIConfig::REQUEST_TIMEOUT = 30;
const int APIConfig::MAX_RETRIES = 3;
const std::string APIConfig::USER_AGENT = "Cockpit-Messenger/1.0"; 