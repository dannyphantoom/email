# Cockpit Messenger

A modern, secure web messenger with email-like addressing (user@cockpit.com) built with C++ backend and React frontend.

## 🚀 Features

### Core Messaging
- **Real-time messaging** with WebSocket support
- **Email-style addressing** (user@cockpit.com)
- **Direct messages** between users
- **Group chats** with member management
- **Message encryption** for secure communication
- **Message history** and persistence

### User Management
- **User registration** and authentication
- **Session management** with JWT tokens
- **Password hashing** with SHA-256
- **Online/offline status** tracking
- **User profiles** and public keys

### Security
- **End-to-end encryption** (AES-256)
- **Secure WebSocket connections**
- **Password hashing** and salting
- **JWT token authentication**
- **SQL injection protection**

### Modern UI
- **Responsive design** with Tailwind CSS
- **Dark theme** with black/blue/purple color scheme
- **Real-time updates** and notifications
- **Modern React** with TypeScript
- **Mobile-friendly** interface

## 🏗️ Architecture

### Backend (C++)
- **Server**: Multi-threaded socket server
- **Database**: SQLite with encrypted storage
- **WebSocket**: Real-time communication
- **Encryption**: OpenSSL integration
- **Authentication**: JWT-based sessions

### Frontend (React + TypeScript)
- **Framework**: React 18 with TypeScript
- **Styling**: Tailwind CSS
- **State Management**: Zustand
- **Routing**: React Router
- **Build Tool**: Vite

## 📦 Installation

### Prerequisites
- Linux/Ubuntu system
- Node.js 18+ and npm
- CMake 3.15+
- OpenSSL development libraries
- SQLite development libraries

### Quick Start
```bash
# Clone and build everything
git clone <repository>
cd email
./build.sh

# Start the backend server
cd backend/build
./cockpit_server

# In another terminal, start the frontend
cd frontend
npm run dev
```

### Manual Installation

#### Backend Setup
```bash
# Install system dependencies
sudo apt update
sudo apt install build-essential libssl-dev libsqlite3-dev cmake

# Build backend
cd backend
mkdir build && cd build
cmake ..
make
```

#### Frontend Setup
```bash
# Install Node.js dependencies
cd frontend
npm install

# Start development server
npm run dev
```

## 🎯 Usage

### Accessing the Application
1. **Frontend**: Open http://localhost:3000 in your browser
2. **Backend API**: Available on http://localhost:8080

### Demo Credentials
- **Username**: demo@cockpit.com
- **Password**: demo123

### Features Walkthrough

#### Registration & Login
1. Visit http://localhost:3000
2. Click "Register" to create a new account
3. Use email-style username (e.g., john@cockpit.com)
4. Set a secure password
5. Login with your credentials

#### Messaging
1. **Direct Messages**: Click on a user in the sidebar to start a conversation
2. **Group Chats**: Create or join group chats for team communication
3. **Real-time**: Messages appear instantly with WebSocket updates

#### Security Features
- All messages are encrypted end-to-end
- Passwords are securely hashed
- Sessions expire automatically
- No message content is stored in plain text

## 🔧 Configuration

### Backend Configuration
The backend server can be configured by modifying the following:

- **Port**: Default 8080 (set in `main.cpp`)
- **Database**: Default `cockpit.db` (set in `main.cpp`)
- **Encryption**: AES-256 with random IVs
- **Session Timeout**: 24 hours

### Frontend Configuration
Frontend settings are in `frontend/vite.config.ts`:

- **Port**: Default 3000
- **API Endpoint**: Configured to connect to backend
- **Theme**: Black/blue/purple color scheme

## 🛠️ Development

### Project Structure
```
email/
├── backend/
│   ├── include/          # Header files
│   ├── src/             # Source files
│   ├── build/           # Build output
│   └── CMakeLists.txt   # Build configuration
├── frontend/
│   ├── src/             # React components
│   ├── public/          # Static assets
│   └── package.json     # Dependencies
├── build.sh             # Build script
└── README.md           # This file
```

### Key Components

#### Backend Components
- **Server**: Main server loop and connection handling
- **Database**: SQLite operations and data persistence
- **WebSocket**: Real-time communication protocol
- **Encryption**: AES-256 encryption and key management
- **Authentication**: JWT token validation and user sessions
- **Message Handler**: Message routing and processing
- **User Manager**: User operations and validation
- **Group Chat**: Group management and permissions

#### Frontend Components
- **App**: Main application with routing
- **Login/Register**: Authentication forms
- **Messenger**: Main messaging interface
- **Sidebar**: User list and navigation
- **ChatWindow**: Message display and input
- **AuthStore**: State management for authentication

### Building from Source
```bash
# Backend
cd backend
mkdir build && cd build
cmake ..
make

# Frontend
cd frontend
npm install
npm run build
```

## 🔒 Security Considerations

### Encryption
- **AES-256** for message encryption
- **Random IVs** for each message
- **Key derivation** with PBKDF2
- **Secure key storage** in memory only

### Authentication
- **JWT tokens** with expiration
- **Password hashing** with SHA-256
- **Session management** with database storage
- **CSRF protection** via tokens

### Data Protection
- **SQL injection prevention** with prepared statements
- **Input validation** on all endpoints
- **Secure headers** and CORS configuration
- **No sensitive data logging**

## 🚀 Deployment

### Production Setup
1. **Backend**: Compile with release flags and run as service
2. **Frontend**: Build optimized version with `npm run build`
3. **Database**: Use production SQLite or migrate to PostgreSQL
4. **SSL**: Configure HTTPS with proper certificates
5. **Firewall**: Restrict access to necessary ports only

### Docker Deployment
```dockerfile
# Backend Dockerfile
FROM ubuntu:22.04
RUN apt update && apt install -y libssl3 libsqlite3-0
COPY cockpit_server /usr/local/bin/
EXPOSE 8080
CMD ["cockpit_server"]
```

## 🐛 Troubleshooting

### Common Issues

#### Backend Won't Start
- Check if port 8080 is available
- Verify OpenSSL and SQLite libraries are installed
- Check database file permissions

#### Frontend Won't Load
- Ensure Node.js 18+ is installed
- Check if port 3000 is available
- Verify all dependencies are installed with `npm install`

#### Connection Issues
- Check firewall settings
- Verify WebSocket support in browser
- Check CORS configuration

### Logs and Debugging
- **Backend logs**: Check console output for error messages
- **Frontend logs**: Open browser developer tools
- **Database**: Check `cockpit.db` file integrity

## 📝 API Documentation

### Authentication Endpoints
- `POST /auth/register` - User registration
- `POST /auth/login` - User login
- `POST /auth/logout` - User logout

### Messaging Endpoints
- `GET /messages/:userId` - Get conversation history
- `POST /messages` - Send message
- `GET /messages/group/:groupId` - Get group messages

### User Management
- `GET /users` - Get all users
- `GET /users/:id` - Get user profile
- `PUT /users/:id/status` - Update online status

### WebSocket Events
- `message` - New message received
- `user_status` - User online/offline status
- `typing` - User typing indicator

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.

## 🙏 Acknowledgments

- **OpenSSL** for cryptographic functions
- **SQLite** for database management
- **React** and **Vite** for frontend framework
- **Tailwind CSS** for styling
- **WebSocket** for real-time communication

---

**Cockpit Messenger** - Secure, modern messaging for the web. 🚀 