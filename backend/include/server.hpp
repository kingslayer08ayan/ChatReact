#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <netinet/in.h>   // sockaddr_in
#include <sys/epoll.h>    // epoll
#include <vector>

namespace ChatServer {

    class Server {
    public:
        Server();
        ~Server();

        // Start the server loop
        void run();

    private:
        int server_fd;     // listening socket
        int epoll_fd;      // epoll instance
        sockaddr_in addr;  // server address

        // Map client socket -> username (for message routing)
        std::unordered_map<int, std::string> clients;
        std::unordered_map<std::string, int> username_fd_map;
        std::unordered_map<std::string, std::string> user_status_map; // username -> status string

        // group_name -> set of usernames
        std::unordered_map<std::string, std::unordered_set<std::string>> groups;

        // group_name -> set of admin usernames
        std::unordered_map<std::string, std::unordered_set<std::string>> group_admins;

        std::unordered_map<std::string, std::unordered_set<std::string>> group_members;
        // Setup
        void initServerSocket();
        void initEpoll();

        // Event handling
        void handleNewConnection();
        void handleClientMessage(int client_fd);
        void removeClient(int client_fd);

        // Utility
        void broadcastMessage(const std::string& msg, int exclude_fd = -1);
        void sendMessage(int client_fd, const std::string& msg);
        void logMessage(const std::string& msg);
    };

} // namespace ChatServer

#endif // SERVER_HPP
