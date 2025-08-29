#include "client.hpp"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <fstream>
#include <sys/stat.h>

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

    return true; // don't send username here, do it from client_main
}

// --- Utility: extract filename ---
static std::string getFilename(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) return path;
    return path.substr(pos + 1);
}

bool Client::sendFile(const std::string& target, const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file '" << filepath << "'\n";
        return false;
    }

    int filesize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string filename = getFilename(filepath);

    std::cout << "Sending file '" << filename << "' (" << filesize << " bytes) to " << target << std::endl;

    // Step 1: notify server (only filename, not full path)
    std::string header = "/sendfile " + target + " " + filename + " " + std::to_string(filesize)+ "\n";
    if (send(sock_fd, header.c_str(), header.size(), 0) == -1) {
        perror("send");
        return false;
    }

    // Step 2: send file contents
    char buffer[1024];
    int remaining = filesize;
    while (remaining > 0) {
        int chunk_size = std::min(remaining, (int)sizeof(buffer));
        file.read(buffer, chunk_size);
        int read_bytes = file.gcount();
        if (read_bytes <= 0) break;

        int sent = send(sock_fd, buffer, read_bytes, 0);
        if (sent <= 0) {
            perror("send file chunk");
            return false;
        }

        remaining -= sent;
    }

    file.close();
    return true;
}


void Client::receiveFile(const std::string &filename, int filesize) {
    std::string save_name = "received_" + filename;  // ✅ prevent overwrite

    std::ofstream outfile(save_name, std::ios::binary);
    if (!outfile.is_open()) {
        std::cerr << "Failed to create file: " << save_name << std::endl;
        return;
    }

    char buffer[1024];
    int remaining = filesize;
    while (remaining > 0) {
        int chunk_size = std::min(remaining, (int)sizeof(buffer));
        int bytes = recv(sock_fd, buffer, chunk_size, 0);
        if (bytes <= 0) break;
        outfile.write(buffer, bytes);
        remaining -= bytes;
    }

    outfile.close();
    std::cout << "File '" << save_name << "' received successfully.\n";
}

void Client::sendMessage(const std::string& msg) {
    send(sock_fd, msg.c_str(), msg.size(), 0);
}

void Client::listen() {
    char buffer[2048];  // bigger buffer just in case

    while (true) {
        int bytes = recv(sock_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            std::cout << "Disconnected from server.\n";
            break;
        }

        buffer[bytes] = '\0';
        std::string msg(buffer);

        // Detect incoming file
        if (msg.rfind("[File incoming]", 0) == 0) {
            std::string filename;
            int filesize = 0;

            size_t pos1 = msg.find("] ");
            size_t pos2 = msg.find(" from ");
            size_t pos3 = msg.find("(");

            if (pos1 != std::string::npos && pos2 != std::string::npos && pos3 != std::string::npos) {
                filename = msg.substr(pos1 + 2, pos2 - (pos1 + 2));
                std::string size_str = msg.substr(pos3 + 1, msg.find(" bytes") - (pos3 + 1));
                filesize = std::stoi(size_str);
            }

            std::cout << msg << std::endl;

            // Now receive actual file
            receiveFile(filename, filesize);
            continue;
        }

        // Regular chat
        std::cout << msg << std::endl;
    }
}

std::string Client::receiveMessage() {
    char buffer[1024];
    int bytes = recv(sock_fd, buffer, sizeof(buffer), 0);
    if (bytes <= 0) return "";
    return std::string(buffer, bytes);
}

} // namespace ChatServer
