#include "Message.h"
#include <sstream>
#include <iomanip>
#include <random>

Message::Message() 
    : timestamp(std::time(nullptr)), ttl(10), is_acknowledgment(false) {}

Message::Message(const std::string& group_id, const std::string& sender_id, const std::string& content)
    : message_id(generateMessageId()), group_id(group_id), sender_id(sender_id),
      timestamp(std::time(nullptr)), content(content), ttl(10), is_acknowledgment(false) {}

std::string Message::serialize() const {
    std::stringstream ss;
    ss << message_id << "|" << group_id << "|" << sender_id << "|"
       << timestamp << "|" << content << "|" << signature << "|" << ttl;
    return ss.str();
}

Message Message::deserialize(const std::string& serialized) {
    std::istringstream ss(serialized);
    Message msg;
    std::string line;
    
    // Check if it's a simple acknowledgment
    if (serialized == "Message received") {
        msg.setContent(serialized);
        msg.setAsAcknowledgment(true);
        return msg;
    }
    
    // Find the start of the actual message (should start with a valid message_id)
    while (std::getline(ss, line, '|')) {
        if (line.length() == 32 && line.find_first_not_of("0123456789abcdef") == std::string::npos) {
            msg.message_id = line;
            break;
        }
    }

    if (msg.message_id.empty()) {
        // This might be a system message
        msg.setContent(serialized);
        msg.setAsAcknowledgment(true);
        return msg;
    }

    std::getline(ss, msg.group_id, '|');
    std::getline(ss, msg.sender_id, '|');
    std::string timestamp_str;
    std::getline(ss, timestamp_str, '|');
    std::getline(ss, msg.content, '|');
    std::getline(ss, msg.signature, '|');
    ss >> msg.ttl;

    if (ss.fail()) {
        throw std::runtime_error("Failed to parse message: " + serialized);
    }

    msg.timestamp = std::stoll(timestamp_str);
    return msg;
}

std::string Message::generateMessageId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    const char* hex_chars = "0123456789abcdef";
    std::string uuid;
    for (int i = 0; i < 32; ++i) {
        uuid += hex_chars[dis(gen)];
    }
    return uuid;
}

// Add these new methods to align with the updated Message.h

void Message::setContent(const std::string& new_content) {
    content = new_content;
}

