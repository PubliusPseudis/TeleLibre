#ifndef PACKET_H
#define PACKET_H

#include <vector>
#include <cstdint>
#include <string>

const uint32_t MAGIC_NUMBER = 0x54454C45;  // "TELE" in ASCII

struct Packet {
    uint32_t magic;           // Magic number to identify start of packet (e.g., 0x54454C45 for "TELE")
    uint32_t length;          // Length of the payload
    uint32_t sequence;        // Sequence number for ordering packets
    uint32_t checksum;        // CRC32 checksum of the payload
    std::vector<uint8_t> payload;  // Actual message content
};


Packet createPacket(const std::string& message, uint32_t sequence);
std::vector<uint8_t> serializePacket(const Packet& packet);
Packet deserializePacket(const std::vector<uint8_t>& data);
uint32_t calculateCRC32(const std::vector<uint8_t>& data);

#endif // PACKET_H
