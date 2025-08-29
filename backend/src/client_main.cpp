#include "client.hpp"
#include <thread>
#include <iostream>
#include <string>
#include <atomic>

std::atomic<bool> running(true);

void receiveMessages(ChatServer::Client& client) {
    while (running) {
        std::string msg = client.receiveMessage();
        if (!msg.empty()) {
            // Print message and keep input prompt intact
            std::cout << "\r" << msg << "\n> ";
            std::cout.flush();
        }
    }
}

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

    // Start receiver thread
    std::thread receiver(receiveMessages, std::ref(client));

    std::string input;
    while (running) {
        std::cout << "> ";
        std::getline(std::cin, input);

        if (input == "/quit") {
            running = false;
            break;
        }

        if (!input.empty()) {
            client.sendMessage(input);
        }
    }

    // Gracefully shutdown
    client.sendMessage(username + " has left the chat.");
    receiver.join(); // wait for receiver thread to finish
    std::cout << "Disconnected from server." << std::endl;
    return 0;
}
