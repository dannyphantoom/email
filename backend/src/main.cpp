#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
#include "server.h"

std::unique_ptr<Server> server;

void signalHandler(int signum) {
    std::cout << "\nReceived signal " << signum << ". Shutting down server..." << std::endl;
    if (server) {
        server->stop();
    }
    exit(0);
}

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [OPTIONS]\n"
              << "Options:\n"
              << "  -p, --port PORT        Server port (default: 8080)\n"
              << "  -d, --database PATH    Database file path (default: cockpit.db)\n"
              << "  -i, --init-db          Initialize database\n"
              << "  -h, --help             Show this help message\n"
              << "  -v, --version          Show version information\n"
              << std::endl;
}

void printVersion() {
    std::cout << "Cockpit Messenger Server v1.0.0\n"
              << "A modern, encrypted web messenger\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    int port = 8080;
    std::string dbPath = "cockpit.db";
    bool initDb = false;
    
    // Parse command line arguments
    static struct option long_options[] = {
        {"port", required_argument, 0, 'p'},
        {"database", required_argument, 0, 'd'},
        {"init-db", no_argument, 0, 'i'},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    
    while ((opt = getopt_long(argc, argv, "p:d:ihv", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'p':
                port = std::stoi(optarg);
                break;
            case 'd':
                dbPath = optarg;
                break;
            case 'i':
                initDb = true;
                break;
            case 'h':
                printUsage(argv[0]);
                return 0;
            case 'v':
                printVersion();
                return 0;
            default:
                printUsage(argv[0]);
                return 1;
        }
    }
    
    // Set up signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    std::cout << "Starting Cockpit Messenger Server..." << std::endl;
    std::cout << "Port: " << port << std::endl;
    std::cout << "Database: " << dbPath << std::endl;
    
    try {
        // Create and initialize server
        server = std::make_unique<Server>(port);
        
        if (!server->initialize()) {
            std::cerr << "Failed to initialize server" << std::endl;
            return 1;
        }
        
        if (initDb) {
            std::cout << "Database initialized successfully" << std::endl;
            return 0;
        }
        
        std::cout << "Server initialized successfully. Starting..." << std::endl;
        std::cout << "Press Ctrl+C to stop the server" << std::endl;
        
        // Run the server
        server->run();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 