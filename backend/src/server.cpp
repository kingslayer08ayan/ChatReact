#include "server.hpp"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <algorithm>
#include <fstream>
#include <chrono>
#include <ctime>
#include <csignal>
#include <sstream>
#include <filesystem>
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

void Server::logMessage(const std::string& msg) {
    std::ofstream log("chat.log", std::ios::app);
    if (log.is_open()) {
        log << msg << std::endl;
    }
}

void Server::handleClientMessage(int client_fd) {
    char buffer[1024];
    int bytes_read = read(client_fd, buffer, sizeof(buffer));
    if (bytes_read <= 0) {
        if (!clients[client_fd].empty()) logMessage("Client disconnected: " + clients[client_fd]);
        removeClient(client_fd);
        return;
    }

    std::string msg = trim(std::string(buffer, bytes_read));
    std::string sender = clients[client_fd];

    // First message = username
    if (sender.empty()) {
        std::string new_username = msg;

        // Force logout if username already logged in
        if (username_fd_map.count(new_username)) {
            int old_fd = username_fd_map[new_username];
            sendMessage(old_fd, "You have been logged out: same username logged in elsewhere.");
            logMessage("Client " + std::to_string(old_fd) + " forcefully logged out for username: " + new_username);
            removeClient(old_fd);
        }

        clients[client_fd] = new_username;
        username_fd_map[new_username] = client_fd;

        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        user_status_map[new_username] = "online since " + std::string(std::ctime(&now));
        user_status_map[new_username].pop_back(); // remove newline

        std::cout << "Client " << client_fd << " set username to " << new_username << std::endl;
        logMessage("Client " + std::to_string(client_fd) + " set username to " + new_username);
        sendMessage(client_fd, "Welcome, " + new_username + "!");
        return;
    }

    // Commands
    if (msg == "/help") {
        sendMessage(client_fd,
            "Available commands:\n"
            "/msg <user> <message>\n"
            "/list\n"
            "/whoami\n"
            "/creategroup <group_name>\n"
            "/addmember <group_name> <username>\n"
            "/kickmember <group_name> <username>\n"
            "/listgroups\n"
            "/gmsg <group_name> <message>\n"
            "/sendfile <user> <filename> <filesize>\n"
            "/quit");
        return;
    }

    if (msg == "/whoami") {
        sendMessage(client_fd, "You are logged in as: " + sender);
        return;
    }

    if (msg == "/list") {
        std::string list_text = "Online users:\n";
        for (auto& [username, status] : user_status_map)
            list_text += username + " (" + status + ")\n";
        sendMessage(client_fd, list_text);
        return;
    }

    // Private message
    if (msg.rfind("/msg ", 0) == 0) {
        size_t space_pos = msg.find(' ', 5);
        if (space_pos == std::string::npos) { sendMessage(client_fd, "Error: Usage: /msg <user> <message>"); return; }
        std::string target = trim(msg.substr(5, space_pos - 5));
        std::string private_msg = trim(msg.substr(space_pos + 1));
        if (target.empty() || private_msg.empty()) { sendMessage(client_fd, "Error: Usage: /msg <user> <message>"); return; }
        if (target == sender) { sendMessage(client_fd, "Error: Cannot message yourself."); return; }

        auto it = username_fd_map.find(target);
        if (it == username_fd_map.end()) { sendMessage(client_fd, "Error: User '" + target + "' not found."); return; }
        int target_fd = it->second;

        sendMessage(target_fd, "[Private] " + sender + ": " + private_msg);
        sendMessage(client_fd, "[Private to " + target + "] " + private_msg);
        logMessage("[Private] " + sender + " -> " + target + ": " + private_msg);
        return;
    }

    // Group creation
    if (msg.rfind("/creategroup ", 0) == 0) {
        std::string group_name = trim(msg.substr(13));
        if (group_name.empty()) { sendMessage(client_fd, "Error: Usage: /creategroup <group_name>"); return; }
        if (groups.count(group_name)) { sendMessage(client_fd, "Error: Group already exists."); return; }
        groups[group_name].insert(sender);
        group_admins[group_name].insert(sender);
        sendMessage(client_fd, "Group '" + group_name + "' created. You are admin.");
        return;
    }

    // Add member
    if (msg.rfind("/addmember ", 0) == 0) {
        size_t space_pos = msg.find(' ', 11);
        if (space_pos == std::string::npos) { sendMessage(client_fd, "Error: Usage: /addmember <group_name> <username>"); return; }
        std::string group_name = trim(msg.substr(11, space_pos - 11));
        std::string new_user = trim(msg.substr(space_pos + 1));
        if (!groups.count(group_name)) { sendMessage(client_fd, "Error: Group does not exist."); return; }
        if (!group_admins[group_name].count(sender)) { sendMessage(client_fd, "Error: Only admins can add members."); return; }
        groups[group_name].insert(new_user);
        sendMessage(client_fd, "User '" + new_user + "' added to group '" + group_name + "'.");
        return;
    }

    // Kick member
    if (msg.rfind("/kickmember ", 0) == 0) {
        size_t space_pos = msg.find(' ', 12);
        if (space_pos == std::string::npos) { sendMessage(client_fd, "Error: Usage: /kickmember <group_name> <username>"); return; }
        std::string group_name = trim(msg.substr(12, space_pos - 12));
        std::string target_user = trim(msg.substr(space_pos + 1));
        if (!groups.count(group_name)) { sendMessage(client_fd, "Error: Group does not exist."); return; }
        if (!group_admins[group_name].count(sender)) { sendMessage(client_fd, "Error: Only admins can kick members."); return; }
        if (!groups[group_name].count(target_user)) { sendMessage(client_fd, "Error: User is not in the group."); return; }
        groups[group_name].erase(target_user);
        group_admins[group_name].erase(target_user);
        sendMessage(client_fd, "User '" + target_user + "' removed from group '" + group_name + "'.");
        return;
    }

    // List groups
    if (msg == "/listgroups") {
        std::string response = "Groups you are in:\n";
        for (auto& [grp, members] : groups) {
            if (members.count(sender)) {
                response += grp + " (Admins: ";
                for (auto& admin : group_admins[grp]) response += admin + " ";
                response += ")\nMembers: ";
                for (auto& mem : members) response += mem + " ";
                response += "\n";
            }
        }
        sendMessage(client_fd, response);
        return;
    }

    // Group message
    if (msg.rfind("/gmsg ", 0) == 0) {
        size_t space_pos = msg.find(' ', 6);
        if (space_pos == std::string::npos) { sendMessage(client_fd, "Error: Usage: /gmsg <group_name> <message>"); return; }
        std::string group_name = trim(msg.substr(6, space_pos - 6));
        std::string group_msg = trim(msg.substr(space_pos + 1));
        if (!groups.count(group_name)) { sendMessage(client_fd, "Error: Group '" + group_name + "' does not exist."); return; }
        if (!groups[group_name].count(sender)) { sendMessage(client_fd, "Error: You are not a member of group '" + group_name + "'."); return; }

        for (const auto& member : groups[group_name]) {
            if (username_fd_map.count(member)) {
                int member_fd = username_fd_map[member];
                if (member_fd != client_fd) sendMessage(member_fd, "[Group " + group_name + "] " + sender + ": " + group_msg);
            }
        }
        sendMessage(client_fd, "[Group " + group_name + "] " + sender + ": " + group_msg);
        logMessage("[Group " + group_name + "] " + sender + ": " + group_msg);
        return;
    }

    // File transfer
    // Inside Server::handleClientMessage
    // File transfer
    if (msg.rfind("/sendfile ", 0) == 0) {
        std::istringstream iss(msg.substr(10));
        std::string target, filepath_or_filename;
        std::string remaining_params;
        
        // Parse: <target> <filepath_or_filename> [optional_size]
        if (!(iss >> target >> filepath_or_filename)) {
            sendMessage(client_fd, "Error: Usage: /sendfile <user> <filepath_or_filename> [filesize]");
            return;
        }
        
        // Check if third parameter exists (filesize)
        std::string size_param;
        iss >> size_param;
        
        auto it = username_fd_map.find(target);
        if (it == username_fd_map.end()) {
            sendMessage(client_fd, "Error: User '" + target + "' not found.");
            return;
        }
        
        int target_fd = it->second;
        std::string filename;
        int filesize = 0;
        bool is_filepath = false;
        
        // Determine if it's a file path or just filename with size
        if (size_param.empty()) {
            // No size provided - assume it's a file path, try to read it
            is_filepath = true;
            filename = filepath_or_filename.substr(filepath_or_filename.find_last_of("/\\") + 1);
            
            try {
                // Check if file exists and get size
                if (std::filesystem::exists(filepath_or_filename)) {
                    filesize = std::filesystem::file_size(filepath_or_filename);
                    std::cout << "File found: " << filepath_or_filename << " (" << filesize << " bytes)" << std::endl;
                } else {
                    sendMessage(client_fd, "Error: File '" + filepath_or_filename + "' not found.");
                    return;
                }
            } catch (const std::exception& e) {
                sendMessage(client_fd, "Error: Cannot access file '" + filepath_or_filename + "': " + e.what());
                return;
            }
        } else {
            // Size provided - traditional mode (filename + size, client sends data)
            filename = filepath_or_filename;
            try {
                filesize = std::stoi(size_param);
            } catch (const std::exception& e) {
                sendMessage(client_fd, "Error: Invalid file size '" + size_param + "'");
                return;
            }
        }
        
        // Notify receiver
        sendMessage(target_fd, "[File incoming] " + filename + " from " + clients[client_fd] +
                            " (" + std::to_string(filesize) + " bytes)");
        
        if (is_filepath) {
            // Server-side file reading and streaming
            std::ifstream file(filepath_or_filename, std::ios::binary);
            if (!file.is_open()) {
                sendMessage(client_fd, "Error: Cannot open file '" + filepath_or_filename + "'");
                sendMessage(target_fd, "Error: File transfer from " + clients[client_fd] + " failed.");
                return;
            }
            
            // Stream file directly from disk to target
            char buffer[1024];
            int remaining = filesize;
            
            while (remaining > 0 && file.good()) {
                int chunk_size = std::min((int)sizeof(buffer), remaining);
                file.read(buffer, chunk_size);
                int bytes_read = file.gcount();
                
                if (bytes_read <= 0) break;
                
                int bytes_sent = send(target_fd, buffer, bytes_read, 0);
                if (bytes_sent <= 0) {
                    sendMessage(client_fd, "Error: Failed to send file data to " + target);
                    sendMessage(target_fd, "Error: File transfer from " + clients[client_fd] + " interrupted.");
                    file.close();
                    return;
                }
                
                remaining -= bytes_sent;
            }
            
            file.close();
            
            if (remaining > 0) {
                sendMessage(client_fd, "Error: File transfer incomplete.");
                sendMessage(target_fd, "Error: File transfer from " + clients[client_fd] + " incomplete.");
                return;
            }
            
            // Success
            sendMessage(client_fd, "File '" + filename + "' sent successfully to " + target);
            sendMessage(target_fd, "File '" + filename + "' received successfully from " + clients[client_fd]);
            logMessage("File transfer: " + clients[client_fd] + " -> " + target + " (" + filename + ", " + std::to_string(filesize) + " bytes)");
            
        } else {
            // Traditional mode - expect client to send file data
            // Look for leftover bytes after header
            size_t newline_pos = msg.find('\n');
            std::string leftover;
            if (newline_pos != std::string::npos) {
                leftover = msg.substr(newline_pos + 1);
            }
            
            char buffer[1024];
            int remaining = filesize;
            
            // Use leftover first
            if (!leftover.empty()) {
                int chunk = std::min((int)leftover.size(), remaining);
                send(target_fd, leftover.data(), chunk, 0);
                remaining -= chunk;
            }
            
            // Stream rest directly from client
            while (remaining > 0) {
                int n = recv(client_fd, buffer, std::min((int)sizeof(buffer), remaining), 0);
                if (n <= 0) {
                    if (remaining > 0) {
                        sendMessage(client_fd, "Error: File transfer interrupted.");
                        sendMessage(target_fd, "Error: File transfer from " + clients[client_fd] + " failed.");
                    }
                    return;
                }
                send(target_fd, buffer, n, 0);
                remaining -= n;
            }
            
            // Success
            sendMessage(client_fd, "File '" + filename + "' sent successfully to " + target);
            sendMessage(target_fd, "File '" + filename + "' received successfully from " + clients[client_fd]);
        }
        
        return;
    }








    // Broadcast normal message
    std::cout << sender << ": " << msg << std::endl;
    broadcastMessage(sender + ": " + msg, client_fd);
    logMessage(sender + ": " + msg);
}

void Server::removeClient(int client_fd) {
    std::string name = clients[client_fd];
    if (!name.empty()) {
        username_fd_map.erase(name);

        // mark offline
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        user_status_map[name] = "offline since " + std::string(std::ctime(&now));
        user_status_map[name].pop_back(); // remove newline

        std::cout << "Client disconnected: " << name << std::endl;
        logMessage("Client disconnected: " + name);
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
