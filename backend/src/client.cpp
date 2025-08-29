#include "client.hpp"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>

namespace ChatServer {

Client::Client(const std::string& host, int port)
    : sock_fd(-1), server_host(host), server_port(port), username("") {}

Client::~Client() {
    if (sock_fd != -1) close(sock_fd);
}

bool Client::connectToServer() {
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("socket");
        return false;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_host.c_str(), &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        return false;
    }

    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        return false;
    }

    // Send username to server on connect
    sendMessage(username);

    return true;
}

void Client::sendMessage(const std::string& msg) {
    send(sock_fd, msg.c_str(), msg.size(), 0);
}

std::string Client::receiveMessage() {
    char buffer[1024];
    int bytes = recv(sock_fd, buffer, sizeof(buffer), 0);
    if (bytes <= 0) return "";
    return std::string(buffer, bytes);
}

} // namespace ChatServer
