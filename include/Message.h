#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>
#include <vector>
#include <ctime>
#include "Packet.h"

class Message {
public:
    Message();
    Message(const std::string& group_id, const std::string& sender_id, const std::string& content);

    std::vector<Packet> serialize() const;
    static Message deserialize(const std::vector<Packet>& packets);

    // Getters
    std::string getMessageId() const { return message_id; }
    std::string getGroupId() const { return group_id; }
    std::string getSenderId() const { return sender_id; }
    time_t getTimestamp() const { return timestamp; }
    std::string getContent() const { return content; }
    std::string getSignature() const { return signature; }
    int getTTL() const { return ttl; }
    bool isAcknowledgment() const { return is_acknowledgment; }

    // Setters
    void setContent(const std::string& new_content);
    void setSignature(const std::string& sig);
    void setTTL(int new_ttl);
    void setAsAcknowledgment(bool is_ack);

private:
    std::string message_id;
    std::string group_id;
    std::string sender_id;
    time_t timestamp;
    std::string content;
    std::string signature;
    int ttl;
    bool is_acknowledgment;

    static std::string generateMessageId();
};

#endif // MESSAGE_H