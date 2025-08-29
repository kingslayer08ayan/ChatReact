#include "message.hpp"
#include <sstream>

namespace ChatServer {

std::string Message::serialize() const {
    std::ostringstream oss;
    oss << static_cast<int>(type) << "|" << sender << "|" << content;
    return oss.str();
}

Message Message::deserialize(const std::string& raw) {
    Message msg;
    std::istringstream iss(raw);
    std::string typeStr, senderStr, contentStr;

    std::getline(iss, typeStr, '|');
    std::getline(iss, senderStr, '|');
    std::getline(iss, contentStr, '|');

    msg.type = static_cast<MessageType>(std::stoi(typeStr));
    msg.sender = senderStr;
    msg.content = contentStr;
    return msg;
}

} // namespace ChatServer
