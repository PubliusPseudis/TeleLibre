#ifndef MESSAGE_H
#define MESSAGE_H

#include <string>
#include <vector>
#include <ctime>

class Message {
public:
    Message(); // Default constructor
    Message(const std::string& group_id, const std::string& sender_id, const std::string& content);

    std::string serialize() const;
    static Message deserialize(const std::string& serialized);

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
    void setSignature(const std::string& sig) { signature = sig; }
    void setTTL(int new_ttl) { ttl = new_ttl; }
    void setAsAcknowledgment(bool is_ack) { is_acknowledgment = is_ack; }
    std::string message_id;
    std::string group_id;
    std::string sender_id;
    time_t timestamp;
    std::string content;
    std::string signature;
    int ttl = 0;
    bool is_acknowledgment = false;

private:

    static std::string generateMessageId();
    void setContent(const std::string &new_content);
};

#endif // MESSAGE_H
