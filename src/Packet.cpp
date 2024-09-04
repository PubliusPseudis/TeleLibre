#include "Packet.h"
#include "Debug.h"
#include <sstream>
#include <iomanip>
#include <boost/crc.hpp>


Packet createPacket(const std::string& message, uint32_t sequence) {
    Packet packet;
    packet.magic = MAGIC_NUMBER;
    packet.payload = std::vector<uint8_t>(message.begin(), message.end());
    packet.length = packet.payload.size();
    packet.sequence = sequence;
    packet.checksum = calculateCRC32(packet.payload);
    return packet;
}

std::vector<uint8_t> serializePacket(const Packet& packet) {
    std::vector<uint8_t> serialized;
    serialized.reserve(16 + packet.payload.size());

    auto appendUint32 = [&serialized](uint32_t value) {
        serialized.push_back((value >> 24) & 0xFF);
        serialized.push_back((value >> 16) & 0xFF);
        serialized.push_back((value >> 8) & 0xFF);
        serialized.push_back(value & 0xFF);
    };

    appendUint32(packet.magic);
    appendUint32(packet.length);
    appendUint32(packet.sequence);
    appendUint32(packet.checksum);
    serialized.insert(serialized.end(), packet.payload.begin(), packet.payload.end());

    std::stringstream ss;
    ss << "Raw bytes: ";
    for (int i = 0; i < 16; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(serialized[i]) << " ";
    }
    Debug::log(ss.str());

    Debug::log("Serialized packet: Magic=" + std::to_string(packet.magic) + 
               ", Length=" + std::to_string(packet.length) + 
               ", Sequence=" + std::to_string(packet.sequence) + 
               ", Checksum=" + std::to_string(packet.checksum) + 
               ", Total size=" + std::to_string(serialized.size()));

    return serialized;
}

Packet deserializePacket(const std::vector<uint8_t>& data) {
    if (data.size() < 16) {
        throw std::runtime_error("Invalid packet: too short");
    }

    Packet packet;
    auto readUint32 = [&data](size_t offset) {
        return (static_cast<uint32_t>(data[offset]) << 24) |
               (static_cast<uint32_t>(data[offset + 1]) << 16) |
               (static_cast<uint32_t>(data[offset + 2]) << 8) |
               static_cast<uint32_t>(data[offset + 3]);
    };

    packet.magic = readUint32(0);
    if (packet.magic != MAGIC_NUMBER) {
        throw std::runtime_error("Invalid packet: wrong magic number");
    }

    packet.length = readUint32(4);
    packet.sequence = readUint32(8);
    packet.checksum = readUint32(12);

    if (data.size() != 16 + packet.length) {
        throw std::runtime_error("Invalid packet: length mismatch");
    }

    packet.payload = std::vector<uint8_t>(data.begin() + 16, data.end());

    uint32_t calculatedChecksum = calculateCRC32(packet.payload);
    if (calculatedChecksum != packet.checksum) {
        throw std::runtime_error("Invalid packet: checksum mismatch");
    }

    return packet;
}

uint32_t calculateCRC32(const std::vector<uint8_t>& data) {
    boost::crc_32_type result;
    result.process_bytes(data.data(), data.size());
    return result.checksum();
}