#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <netinet/in.h>

namespace ChatServer {

    class Client {
    public:
        Client(const std::string& host, int port);
        ~Client();

        bool connectToServer();
        void sendMessage(const std::string& msg);
        std::string receiveMessage();

        void setUsername(const std::string& name) { username = name; }
        std::string getUsername() const { return username; }

    private:
        int sock_fd;
        std::string server_host;
        int server_port;
        sockaddr_in server_addr;

        std::string username;  // NEW: store client's name
    };

} // namespace ChatServer

#endif // CLIENT_HPP
