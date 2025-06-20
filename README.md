# 🚀 Cockpit - Unified Messaging Hub

A modern, unified messaging platform that consolidates all your email and messenger accounts into one powerful interface. Built with a C++ backend and React frontend.

## ✨ Features

### 🔗 **Account Integration**
- **Email Services**: Connect Gmail, Outlook, Yahoo Mail, ProtonMail
- **Messenger Platforms**: Connect WhatsApp, Telegram, Facebook Messenger, Twitter DM, Instagram DM
- **Secure Authentication**: OAuth2 integration for secure account connections
- **Account Management**: Enable/disable accounts, view sync status, manage credentials

### 📬 **Unified Inbox**
- **All-in-One View**: See messages from all connected accounts in a single interface
- **Smart Filtering**: Filter by unread, important, email type, or messenger type
- **Advanced Search**: Search across all messages, subjects, and content
- **Message Actions**: Mark as read, star important messages, reply, delete
- **Real-time Sync**: Automatic message synchronization every 30 seconds

### 💬 **Native Messaging**
- **Real-time Chat**: Instant messaging with other Cockpit users
- **Group Chats**: Create and manage group conversations
- **Message Encryption**: End-to-end encryption for secure communications
- **File Sharing**: Share files and attachments in conversations

### 🎨 **Modern UI/UX**
- **Dark Theme**: Beautiful dark interface with gradient accents
- **Responsive Design**: Works seamlessly on desktop and mobile
- **Intuitive Navigation**: Easy switching between messaging and unified inbox
- **Real-time Updates**: Live notifications and message indicators

## 🏗️ Architecture

### Backend (C++)
- **Account Integration Manager**: Handles external service connections
- **Message Synchronization**: Fetches and syncs messages from all connected accounts
- **Unified Message Format**: Standardizes messages from different providers
- **Database Management**: SQLite for storing accounts, messages, and user data
- **WebSocket Server**: Real-time communication for native messaging
- **Encryption**: Secure storage and transmission of sensitive data

### Frontend (React + TypeScript)
- **Account Integration Page**: Connect and manage external accounts
- **Unified Inbox**: Display and manage all messages in one interface
- **Native Messenger**: Real-time chat interface
- **Responsive Components**: Reusable UI components with Tailwind CSS
- **State Management**: Zustand for authentication and app state

## 🚀 Getting Started

### Prerequisites
- Node.js 18+ and npm
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.15+
- OpenSSL development libraries
- SQLite3 development libraries

### Installation

1. **Clone the repository**
   ```bash
   git clone <repository-url>
   cd cockpit
   ```

2. **Install frontend dependencies**
   ```bash
   cd frontend
   npm install
   ```

3. **Build the backend**
   ```bash
   cd ../backend
   mkdir build && cd build
   cmake ..
   make -j$(nproc)
   ```

4. **Start the servers**
   ```bash
   # Terminal 1: Start backend server
   cd backend/build
   ./cockpit_server

   # Terminal 2: Start frontend development server
   cd frontend
   npm run dev
   ```

5. **Access the application**
   - Frontend: http://localhost:3000
   - Backend API: http://localhost:8080

## 📱 Usage

### 1. **Account Integration**
- Navigate to **Account Integration** (Settings icon in header)
- Click on any email or messenger provider
- Enter your credentials to connect your account
- View connected accounts and their sync status

### 2. **Unified Inbox**
- Access the **Unified Inbox** (Inbox icon in header)
- View all messages from connected accounts
- Use filters to find specific messages
- Search across all your messages
- Take actions: mark as read, star, reply, delete

### 3. **Native Messaging**
- Use the main **Messenger** interface for Cockpit-to-Cockpit chat
- Create new conversations with other users
- Join or create group chats
- Share files and media

## 🔧 Configuration

### Environment Variables
```bash
# Backend Configuration
COCKPIT_DB_PATH=./cockpit.db
COCKPIT_PORT=8080
COCKPIT_SSL_CERT=./cert.pem
COCKPIT_SSL_KEY=./key.pem

# Frontend Configuration
VITE_API_URL=http://localhost:8080
VITE_WS_URL=ws://localhost:8080
```

### Supported Providers

#### Email Services
- **Gmail**: OAuth2 authentication, IMAP/SMTP
- **Outlook**: OAuth2 authentication, Microsoft Graph API
- **Yahoo Mail**: OAuth2 authentication, IMAP/SMTP
- **ProtonMail**: API authentication, encrypted email

#### Messenger Platforms
- **WhatsApp**: WhatsApp Web API integration
- **Telegram**: Telegram Bot API
- **Facebook Messenger**: Facebook Graph API
- **Twitter DM**: Twitter API v2
- **Instagram DM**: Instagram Basic Display API

## 🔒 Security

- **End-to-End Encryption**: All native messages are encrypted
- **OAuth2 Authentication**: Secure third-party account connections
- **Token Management**: Secure storage and refresh of access tokens
- **Data Privacy**: Local message storage with optional cloud sync
- **SSL/TLS**: Encrypted communication between client and server

## 🛠️ Development

### Project Structure
```
cockpit/
├── backend/
│   ├── include/           # Header files
│   ├── src/              # Source files
│   ├── CMakeLists.txt    # Build configuration
│   └── build.sh          # Build script
├── frontend/
│   ├── src/
│   │   ├── components/   # React components
│   │   ├── pages/        # Page components
│   │   ├── stores/       # State management
│   │   └── App.tsx       # Main app component
│   ├── package.json      # Dependencies
│   └── vite.config.ts    # Build configuration
└── README.md
```

### Adding New Providers

1. **Backend Integration**
   ```cpp
   // Add provider type in account_integration.h
   enum class ProviderType {
       // ... existing providers
       NEW_PROVIDER
   };

   // Implement fetch methods in account_integration.cpp
   std::vector<UnifiedMessage> AccountIntegrationManager::fetchNewProviderMessages(const AccountCredentials& account) {
       // Implementation
   }
   ```

2. **Frontend Integration**
   ```typescript
   // Add to AccountIntegration.tsx
   const newProvider = {
     name: 'New Provider',
     icon: NewIcon,
     color: 'bg-blue-500'
   };
   ```

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🆘 Support

- **Documentation**: Check the code comments and this README
- **Issues**: Report bugs and feature requests via GitHub Issues
- **Discussions**: Join the community discussions for help and ideas

## 🚀 Roadmap

- [ ] **Mobile App**: React Native version for iOS and Android
- [ ] **Advanced Filters**: Custom filter rules and automation
- [ ] **Message Templates**: Pre-written responses and templates
- [ ] **Calendar Integration**: Meeting scheduling and reminders
- [ ] **AI Assistant**: Smart message categorization and responses
- [ ] **Cloud Sync**: Optional cloud storage for messages
- [ ] **API Access**: Public API for third-party integrations

---

**Cockpit** - Unifying your digital communications in one powerful platform! 🚀 