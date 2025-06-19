#!/bin/bash
set -e

echo "🚀 Building Cockpit Messenger..."

# Check if required tools are installed
check_command() {
    if ! command -v $1 &> /dev/null; then
        echo "❌ $1 is not installed. Please install it first."
        exit 1
    fi
}

echo "📋 Checking prerequisites..."
check_command "gcc"
check_command "cmake"
check_command "node"
check_command "npm"

# Install system dependencies (Ubuntu/Debian)
if command -v apt-get &> /dev/null; then
    echo "📦 Installing system dependencies..."
    sudo apt-get update
    sudo apt-get install -y libssl-dev libsqlite3-dev build-essential
fi

# Kill any running backend/frontend servers
pkill -f cockpit_server || true
pkill -f "vite" || true

# Build backend
echo "🔨 Building backend..."
cd backend

# Create build directory
mkdir -p build
cd build

# Configure with CMake
echo "📐 Configuring with CMake..."
cmake ..

if [ $? -ne 0 ]; then
    echo "❌ CMake configuration failed!"
    exit 1
fi

# Build
echo "🔨 Compiling..."
make -j$(nproc)

if [ $? -eq 0 ]; then
    echo "✅ Backend built successfully!"
else
    echo "❌ Backend build failed!"
    exit 1
fi

cd ../..

# Install frontend dependencies
echo "🎨 Installing frontend dependencies..."
cd frontend
npm install
# Skip build step for dev mode - just run dev server
cd ..

# Start backend server in background
./backend/build/cockpit_server &
BACKEND_PID=$!
echo "Started backend (PID $BACKEND_PID) on http://localhost:8080"

# Start frontend dev server in background
cd frontend
nohup npm run dev > ../frontend-dev.log 2>&1 &
FRONTEND_PID=$!
echo "Started frontend (PID $FRONTEND_PID) on http://localhost:3000"
cd ..

# Print summary
cat <<EOF

Cockpit Messenger is running!
- Backend:  http://localhost:8080
- Frontend: http://localhost:3000

To stop servers: kill $BACKEND_PID $FRONTEND_PID
Logs: frontend/frontend-dev.log
EOF

echo ""
echo "🎉 Build completed successfully!"
echo ""
echo "To start the application:"
echo "1. Start the backend server:"
echo "   cd backend/build && ./cockpit_server"
echo ""
echo "2. In another terminal, start the frontend:"
echo "   cd frontend && npm run dev"
echo ""
echo "3. Open your browser to http://localhost:3000"
echo ""
echo "Happy messaging! 🚀" 