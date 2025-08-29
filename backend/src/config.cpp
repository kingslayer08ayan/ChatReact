#include "config.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>  // header-only JSON library

using json = nlohmann::json;

namespace ChatConfig {

    // Defaults (overridden by config.json if provided)
    int SERVER_PORT = 8080;
    int MAX_EVENTS = 1000;
    int BACKLOG = 50;
    int BUFFER_SIZE = 4096;

    int MAX_USERNAME_LEN = 32;
    int MAX_MESSAGE_LEN = 1024;

    int EPOLL_TIMEOUT = 1000;                 // 1 sec
    int CLIENT_INACTIVITY_TIMEOUT = 300000;   // 5 min

    std::string LOG_FILE = "chatserver.log";

    void loadConfig(const std::string &filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "[Config] Could not open config file: " << filename
                      << ". Using defaults." << std::endl;
            return;
        }

        try {
            json j;
            file >> j;

            if (j.contains("port")) SERVER_PORT = j["port"];
            if (j.contains("max_events")) MAX_EVENTS = j["max_events"];
            if (j.contains("backlog")) BACKLOG = j["backlog"];
            if (j.contains("buffer_size")) BUFFER_SIZE = j["buffer_size"];

            if (j.contains("max_username_len")) MAX_USERNAME_LEN = j["max_username_len"];
            if (j.contains("max_message_len")) MAX_MESSAGE_LEN = j["max_message_len"];

            if (j.contains("epoll_timeout")) EPOLL_TIMEOUT = j["epoll_timeout"];
            if (j.contains("client_inactivity_timeout"))
                CLIENT_INACTIVITY_TIMEOUT = j["client_inactivity_timeout"];

            if (j.contains("log_file")) LOG_FILE = j["log_file"];

        } catch (std::exception &e) {
            std::cerr << "[Config] Error parsing config: " << e.what()
                      << ". Using defaults." << std::endl;
        }
    }

}
