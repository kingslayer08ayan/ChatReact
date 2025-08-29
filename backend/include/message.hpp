#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>

namespace ChatServer {

    enum class MessageType {
        TEXT,
        JOIN,
        LEAVE
    };

    struct Message {
        MessageType type;
        std::string sender;
        std::string content;

        // Serialize to string for sending
        std::string serialize() const;

        // Deserialize from string
        static Message deserialize(const std::string& raw);
    };

} // namespace ChatServer

#endif // MESSAGE_HPP
