#include "server.hpp"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <algorithm>

namespace ChatServer {

// Helper to trim whitespace
static std::string trim(const std::string& s) {
    auto start = std::find_if_not(s.begin(), s.end(), ::isspace);
    auto end   = std::find_if_not(s.rbegin(), s.rend(), ::isspace).base();
    if (start >= end) return "";
    return std::string(start, end);
}

Server::Server() {
    initServerSocket();
    initEpoll();
}

Server::~Server() {
    close(server_fd);
    close(epoll_fd);
}

void Server::initServerSocket() {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) { perror("socket"); exit(EXIT_FAILURE); }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(12345);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); exit(EXIT_FAILURE); }
    if (listen(server_fd, SOMAXCONN) < 0) { perror("listen"); exit(EXIT_FAILURE); }

    fcntl(server_fd, F_SETFL, fcntl(server_fd, F_GETFL, 0) | O_NONBLOCK);

    std::cout << "Starting Chat Server on port " << ntohs(addr.sin_port) << "...\n";
}

void Server::initEpoll() {
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) { perror("epoll_create1"); exit(EXIT_FAILURE); }

    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1) { perror("epoll_ctl"); exit(EXIT_FAILURE); }
}

void Server::run() {
    constexpr int MAX_EVENTS = 10;
    epoll_event events[MAX_EVENTS];

    while (true) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) { perror("epoll_wait"); continue; }

        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == server_fd) handleNewConnection();
            else handleClientMessage(events[i].data.fd);
        }
    }
}

void Server::handleNewConnection() {
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd == -1) { perror("accept"); return; }

    fcntl(client_fd, F_SETFL, fcntl(client_fd, F_GETFL, 0) | O_NONBLOCK);

    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = client_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);

    clients[client_fd] = ""; // username not yet set
    std::cout << "New client connected: " << client_fd << std::endl;
}

void Server::handleClientMessage(int client_fd) {
    char buffer[1024];
    int bytes_read = read(client_fd, buffer, sizeof(buffer));
    if (bytes_read <= 0) {
        removeClient(client_fd);
        return;
    }

    std::string msg = trim(std::string(buffer, bytes_read));

    // First message = username
    if (clients[client_fd].empty()) {
        std::string new_username = msg;

        // Check if username is already logged in
        if (username_fd_map.count(new_username)) {
            int old_fd = username_fd_map[new_username];
            sendMessage(old_fd, "You have been logged out because the same username logged in elsewhere.");
            removeClient(old_fd);
            std::cout << "Client " << old_fd << " forcefully logged out for username: " << new_username << std::endl;
        }

        // Register new session
        clients[client_fd] = new_username;
        username_fd_map[new_username] = client_fd;

        std::cout << "Client " << client_fd << " set username to " << new_username << std::endl;
        sendMessage(client_fd, "Welcome, " + new_username + "!");
        return;
    }

    std::string sender = clients[client_fd];

    // Handle /msg command
    if (msg.rfind("/msg ", 0) == 0) {
        size_t space_pos = msg.find(' ', 5);
        if (space_pos == std::string::npos) {
            sendMessage(client_fd, "Error: Usage: /msg <user> <message>");
            return;
        }

        std::string target = trim(msg.substr(5, space_pos - 5));
        std::string private_msg = trim(msg.substr(space_pos + 1));

        if (target.empty() || private_msg.empty()) {
            sendMessage(client_fd, "Error: Usage: /msg <user> <message>");
            return;
        }

        if (target == sender) {
            sendMessage(client_fd, "Error: You cannot message yourself.");
            return;
        }

        // Find recipient fd
        auto it = username_fd_map.find(target);
        if (it == username_fd_map.end()) {
            sendMessage(client_fd, "Error: User '" + target + "' not found.");
            return;
        }

        int target_fd = it->second;

        // Send private message
        sendMessage(target_fd, "[Private] " + sender + ": " + private_msg);
        sendMessage(client_fd, "[Private to " + target + "] " + private_msg);
        return;
    }

    // Normal broadcast
    std::cout << sender << ": " << msg << std::endl;
    broadcastMessage(sender + ": " + msg, client_fd);
}
void Server::removeClient(int client_fd) {
    std::string name = clients[client_fd];
    if (!name.empty()) {
        username_fd_map.erase(name);
        std::cout << "Client disconnected: " << name << std::endl;
    }
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
    close(client_fd);
    clients.erase(client_fd);
}


void Server::broadcastMessage(const std::string& msg, int exclude_fd) {
    for (auto& [fd, name] : clients) {
        if (fd != exclude_fd) sendMessage(fd, msg);
    }
}

void Server::sendMessage(int client_fd, const std::string& msg) {
    send(client_fd, msg.c_str(), msg.size(), 0);
}

} // namespace ChatServer
