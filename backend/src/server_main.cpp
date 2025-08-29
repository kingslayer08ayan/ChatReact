#include "server.hpp"
#include <iostream>
#include <csignal>

ChatServer::Server* g_server = nullptr;

// Optional: Handle Ctrl+C to gracefully shut down server
void signalHandler(int signum) {
    std::cout << "\nShutting down server..." << std::endl;
    if (g_server) {
        delete g_server;
        g_server = nullptr;
    }
    exit(EXIT_SUCCESS);
}

int main() {
    std::signal(SIGINT, signalHandler);

    try {
        ChatServer::Server server;
        g_server = &server;

        std::cout << "Starting Chat Server on port 12345..." << std::endl;

        server.run();

    } catch (const std::exception& e) {
        std::cerr << "Server exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
