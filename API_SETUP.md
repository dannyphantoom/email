# API Integration Setup Guide

This guide will help you set up the Gmail, WhatsApp, and Telegram integrations for your Cockpit messenger app.

## Gmail OAuth2 Setup

### 1. Create Google Cloud Project
1. Go to [Google Cloud Console](https://console.cloud.google.com/)
2. Create a new project or select an existing one
3. Enable the Gmail API:
   - Go to "APIs & Services" > "Library"
   - Search for "Gmail API" and enable it

### 2. Create OAuth2 Credentials
1. Go to "APIs & Services" > "Credentials"
2. Click "Create Credentials" > "OAuth 2.0 Client IDs"
3. Choose "Web application" as the application type
4. Add authorized redirect URIs:
   - `http://localhost:8080/oauth/gmail/callback` (for development)
   - `https://yourdomain.com/oauth/gmail/callback` (for production)
5. Note down your Client ID and Client Secret

### 3. Update Configuration
Edit `backend/src/config.cpp` and replace the placeholder values:

```cpp
const std::string GmailConfig::CLIENT_ID = "your-actual-client-id";
const std::string GmailConfig::CLIENT_SECRET = "your-actual-client-secret";
```

## WhatsApp Web API Setup

### Option 1: Using WhatsApp Business API (Recommended)
1. Apply for WhatsApp Business API access at [Meta for Developers](https://developers.facebook.com/docs/whatsapp)
2. Create a WhatsApp Business App
3. Get your access token and phone number ID
4. Update the configuration in `backend/src/config.cpp`

### Option 2: Using Third-party WhatsApp Web Libraries
For development/testing, you can use libraries like:
- [whatsapp-web.js](https://github.com/pedroslopez/whatsapp-web.js)
- [Baileys](https://github.com/WhiskeySockets/Baileys)

### 3. Update Configuration
Edit `backend/src/config.cpp`:

```cpp
// For WhatsApp Business API
const std::string WhatsAppConfig::API_BASE_URL = "https://graph.facebook.com/v17.0";
const std::string WhatsAppConfig::SESSION_ENDPOINT = "/messages";

// For WhatsApp Web (if using third-party service)
// const std::string WhatsAppConfig::API_BASE_URL = "https://your-whatsapp-api.com";
```

## Telegram Bot API Setup

### 1. Create a Telegram Bot
1. Open Telegram and search for [@BotFather](https://t.me/botfather)
2. Send `/newbot` command
3. Follow the instructions to create your bot
4. Save the bot token (format: `123456789:ABCdefGHIjklMNOpqrsTUVwxyz`)

### 2. Get Chat ID
1. Start a conversation with your bot
2. Send any message to the bot
3. Access this URL in your browser:
   ```
   https://api.telegram.org/bot<YOUR_BOT_TOKEN>/getUpdates
   ```
4. Look for the "chat" object and note the "id" field

### 3. Update Configuration
The Telegram configuration is already set up in `backend/src/config.cpp`:

```cpp
const std::string TelegramConfig::API_BASE_URL = "https://api.telegram.org";
const std::string TelegramConfig::BOT_API_URL = "https://api.telegram.org/bot";
```

## Environment Variables (Optional)

For better security, you can use environment variables instead of hardcoding values:

```bash
# Add to your .env file or system environment
GMAIL_CLIENT_ID=your-gmail-client-id
GMAIL_CLIENT_SECRET=your-gmail-client-secret
WHATSAPP_API_KEY=your-whatsapp-api-key
TELEGRAM_BOT_TOKEN=your-telegram-bot-token
```

Then update `backend/src/config.cpp` to read from environment:

```cpp
#include <cstdlib>

const std::string GmailConfig::CLIENT_ID = std::getenv("GMAIL_CLIENT_ID") ? std::getenv("GMAIL_CLIENT_ID") : "YOUR_GMAIL_CLIENT_ID";
const std::string GmailConfig::CLIENT_SECRET = std::getenv("GMAIL_CLIENT_SECRET") ? std::getenv("GMAIL_CLIENT_SECRET") : "YOUR_GMAIL_CLIENT_SECRET";
```

## Testing the Integrations

### Gmail OAuth2
1. Start the backend server
2. Navigate to the Account Integration page
3. Click "Connect Gmail"
4. You'll be redirected to Google's OAuth consent screen
5. After authorization, you'll be redirected back with a code
6. The backend will exchange the code for access and refresh tokens

### WhatsApp Web
1. Use the WhatsApp Web API endpoint
2. Provide your session ID or API credentials
3. The system will test the connection and fetch messages

### Telegram Bot
1. Use the Telegram Bot API endpoint
2. Provide your bot token and chat ID
3. The system will test the connection and fetch messages

## Troubleshooting

### Gmail Issues
- **"Invalid credentials"**: Make sure you're using OAuth2, not basic auth
- **"Access denied"**: Check that Gmail API is enabled in Google Cloud Console
- **"Redirect URI mismatch"**: Verify the redirect URI in your OAuth2 credentials

### WhatsApp Issues
- **"Session expired"**: WhatsApp Web sessions expire periodically, you'll need to re-authenticate
- **"API rate limits"**: WhatsApp has strict rate limits, implement proper throttling
- **"Unauthorized"**: Check your API credentials and permissions

### Telegram Issues
- **"Bot token invalid"**: Verify your bot token from BotFather
- **"Chat not found"**: Make sure you've started a conversation with your bot
- **"No updates"**: Send a message to your bot to generate updates

## Security Considerations

1. **Never commit API keys to version control**
2. **Use environment variables for sensitive data**
3. **Implement proper token refresh mechanisms**
4. **Add rate limiting to prevent API abuse**
5. **Log API calls for debugging but don't log sensitive data**
6. **Use HTTPS in production**

## Next Steps

1. Implement proper error handling and retry logic
2. Add message sending capabilities
3. Implement real-time updates using webhooks
4. Add support for file attachments
5. Implement message threading and conversations 