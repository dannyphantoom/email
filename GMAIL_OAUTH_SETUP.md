# Gmail OAuth2 Setup Guide

## Quick Setup for Gmail Integration

The Gmail integration now uses OAuth2 authentication, which is required by Google's security policies. Follow these steps to set up your Gmail account:

### Step 1: Create Google Cloud Project

1. Go to [Google Cloud Console](https://console.cloud.google.com/)
2. Create a new project or select an existing one
3. Enable the Gmail API:
   - Go to "APIs & Services" > "Library"
   - Search for "Gmail API" and click "Enable"

### Step 2: Create OAuth2 Credentials

1. Go to "APIs & Services" > "Credentials"
2. Click "Create Credentials" > "OAuth 2.0 Client IDs"
3. Choose "Web application" as the application type
4. Add authorized redirect URIs:
   - `http://localhost:8080/oauth/gmail/callback` (for development)
5. Note down your **Client ID** and **Client Secret**

### Step 3: Update Configuration

Edit the file `backend/src/config.cpp` and replace the placeholder values:

```cpp
const std::string GmailConfig::CLIENT_ID = "your-actual-client-id";
const std::string GmailConfig::CLIENT_SECRET = "your-actual-client-secret";
```

### Step 4: Rebuild the Backend

Run the build script to apply the changes:

```bash
./build.sh
```

### Step 5: Connect Your Gmail Account

1. Open the Cockpit application in your browser
2. Go to "Account Integration" page
3. Click "Connect Gmail"
4. Enter your Gmail address
5. Click "Continue with OAuth2"
6. Click "Authorize Gmail Access" - this will open Google's authorization page
7. Sign in with your Google account and grant permissions
8. Copy the authorization code from the success page
9. Paste the code in the Cockpit application
10. Click "Connect Account"

### Troubleshooting

**"Invalid credentials" error:**
- Make sure you're using OAuth2, not basic authentication
- Verify your Client ID and Client Secret are correct
- Check that Gmail API is enabled in Google Cloud Console

**"Access denied" error:**
- Ensure you've granted the necessary permissions during OAuth2 flow
- Check that your Google account has Gmail enabled

**"Redirect URI mismatch" error:**
- Verify the redirect URI in your OAuth2 credentials matches exactly: `http://localhost:8080/oauth/gmail/callback`

### Security Notes

- Never commit your Client ID and Client Secret to version control
- For production, use environment variables instead of hardcoding values
- The OAuth2 tokens are automatically refreshed by the application

### What's Changed

- **Before**: Used username/password authentication (no longer supported by Gmail)
- **Now**: Uses OAuth2 authentication (secure, modern, and required by Google)

The OAuth2 flow provides better security and follows Google's current authentication standards. Your Gmail credentials are never stored in the application - only secure access tokens are used. 