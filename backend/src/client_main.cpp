#include "client.hpp"
#include <thread>
#include <iostream>
#include <string>
#include <atomic>

std::atomic<bool> running(true);

int main() {
    std::string username;
    std::cout << "Enter your username: ";
    std::getline(std::cin, username);

    ChatServer::Client client("127.0.0.1", 12345);
    if (!client.connectToServer()) {
        std::cerr << "Failed to connect to server\n";
        return 1;
    }

    // Send username to server
    client.sendMessage(username);
    std::cout << "Username set to " << username << std::endl;

    // Start receiver thread using the new listen() method
    std::thread receiver([&client]() {
        client.listen();   // <- this calls the method you added in client.cpp
    });

    std::string input;
    while (running) {
        std::cout << "> ";
        std::getline(std::cin, input);

        if (input == "/quit") {
            running = false;
            break;
        }

        // Parse /sendfile command
        if (input.rfind("/sendfile ", 0) == 0) {
            size_t space_pos = input.find(' ', 10);
            if (space_pos == std::string::npos) {
                std::cout << "Usage: /sendfile <user> <filepath>\n";
                continue;
            }

            std::string target_user = input.substr(10, space_pos - 10);
            std::string filepath = input.substr(space_pos + 1);

            if (!client.sendFile(target_user, filepath)) {
                std::cerr << "File transfer failed!\n";
            }

            continue;
        }

        if (!input.empty()) {
            client.sendMessage(input);
        }
    }

    // Gracefully shutdown
    client.sendMessage(username + " has left the chat.");
    receiver.join(); // wait for listener thread to finish
    std::cout << "Disconnected from server." << std::endl;
    return 0;
}
