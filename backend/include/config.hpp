#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>

namespace ChatConfig {

    // Networking (defaults can be overridden via JSON)
    extern int SERVER_PORT;                // Default listening port
    extern int MAX_EVENTS;                 // Max events epoll can handle at once
    extern int BACKLOG;                    // Max queued connections
    extern int BUFFER_SIZE;                // Buffer size for read/write

    // User constraints
    extern int MAX_USERNAME_LEN;
    extern int MAX_MESSAGE_LEN;

    // Timeouts (ms)
    extern int EPOLL_TIMEOUT;              // epoll_wait timeout in ms
    extern int CLIENT_INACTIVITY_TIMEOUT;  // Client inactivity timeout (default 5 min)

    // Logging
    extern std::string LOG_FILE;

    // Function to allow dynamic config (optional JSON/env load)
    void loadConfig(const std::string &filename);

} // namespace ChatConfig

#endif // CONFIG_HPP
