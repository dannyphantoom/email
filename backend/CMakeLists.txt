cmake_minimum_required(VERSION 3.15)
project(CockpitMessenger VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenSSL REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(Threads REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/main.cpp
    src/server.cpp
    src/websocket_handler.cpp
    src/database.cpp
    src/user_manager.cpp
    src/message_handler.cpp
    src/encryption.cpp
    src/auth.cpp
    src/group_chat.cpp
)

# Header files
set(HEADERS
    include/server.h
    include/websocket_handler.h
    include/database.h
    include/user_manager.h
    include/message_handler.h
    include/encryption.h
    include/auth.h
    include/group_chat.h
)

# Create executable
add_executable(cockpit_server ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(cockpit_server
    OpenSSL::SSL
    OpenSSL::Crypto
    SQLite::SQLite3
    Threads::Threads
    pthread
)

# Compiler flags
target_compile_options(cockpit_server PRIVATE
    -Wall
    -Wextra
    -g
    -O0
    -DDEBUG
)

# Installation
install(TARGETS cockpit_server DESTINATION bin) 