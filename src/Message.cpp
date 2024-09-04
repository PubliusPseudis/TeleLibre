#include "Message.h"
#include "Debug.h"
#include <sstream>
#include <iomanip>
#include <random>

Message::Message() 
    : timestamp(std::time(nullptr)), ttl(10), is_acknowledgment(false) {}

Message::Message(const std::string& group_id, const std::string& sender_id, const std::string& content)
    : message_id(generateMessageId()), group_id(group_id), sender_id(sender_id),
      timestamp(std::time(nullptr)), content(content), ttl(10), is_acknowledgment(false) {}

std::vector<Packet> Message::serialize() const {
    std::stringstream ss;
    ss << message_id << "|" << group_id << "|" << sender_id << "|"
       << timestamp << "|" << content << "|" << signature << "|" << ttl;
    std::string serialized = ss.str();
    
    // For simplicity, we're creating a single packet. In a real-world scenario,
    // you might want to split large messages into multiple packets.
    return {createPacket(serialized, 0)};
}

Message Message::deserialize(const std::vector<Packet>& packets) {
    if (packets.empty()) {
        throw std::runtime_error("No packets to deserialize");
    }

    // For now, we're assuming a single packet. In the future, you might want to
    // handle multiple packets and reassemble them.
    std::string serialized(packets[0].payload.begin(), packets[0].payload.end());
    
    Debug::log("Deserializing message: " + serialized);
    std::istringstream ss(serialized);
    Message msg;

    std::getline(ss, msg.message_id, '|');
    std::getline(ss, msg.group_id, '|');
    std::getline(ss, msg.sender_id, '|');
    std::string timestamp_str;
    std::getline(ss, timestamp_str, '|');
    std::getline(ss, msg.content, '|');
    std::getline(ss, msg.signature, '|');
    ss >> msg.ttl;

    if (ss.fail()) {
        Debug::log("Failed to parse message");
        throw std::runtime_error("Failed to parse message: " + serialized);
    }

    msg.timestamp = std::stoll(timestamp_str);
    Debug::log("Successfully parsed message");
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

void Message::setContent(const std::string& new_content) {
    content = new_content;
}

void Message::setSignature(const std::string& sig) {
    signature = sig;
}

void Message::setTTL(int new_ttl) {
    ttl = new_ttl;
}

void Message::setAsAcknowledgment(bool is_ack) {
    is_acknowledgment = is_ack;
}