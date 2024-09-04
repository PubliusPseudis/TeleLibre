#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <sstream>
#include "Message.h"
#include "Debug.h"
#include "Packet.h"

using boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket) : socket_(std::move(socket)) {}

    void start() {
        Debug::log("New session started");
        do_read_header();
    }

private:
    void do_read_header() {
        auto self(shared_from_this());
        Debug::log("Reading header");
        boost::asio::async_read(socket_, boost::asio::buffer(header_buffer_),
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec && length == 16) {
                    uint32_t magic = (static_cast<uint32_t>(header_buffer_[0]) << 24) |
                                     (static_cast<uint32_t>(header_buffer_[1]) << 16) |
                                     (static_cast<uint32_t>(header_buffer_[2]) << 8) |
                                     static_cast<uint32_t>(header_buffer_[3]);
                    
                    Debug::log("Received magic number: " + std::to_string(magic) + ", Expected: " + std::to_string(MAGIC_NUMBER));
                    
                    if (magic != MAGIC_NUMBER) {
                        Debug::log("Invalid magic number: " + std::to_string(magic));
                        // Try to resynchronize by reading one byte at a time
                        do_resync();
                        return;
                    }

                    uint32_t payload_length = (static_cast<uint32_t>(header_buffer_[4]) << 24) |
                                              (static_cast<uint32_t>(header_buffer_[5]) << 16) |
                                              (static_cast<uint32_t>(header_buffer_[6]) << 8) |
                                              static_cast<uint32_t>(header_buffer_[7]);
                    Debug::log("Header read successfully. Magic: " + std::to_string(magic) + ", Payload length: " + std::to_string(payload_length));
                    
                    if (payload_length > 1000000) {  // Set a reasonable maximum payload size
                        Debug::log("Payload length too large: " + std::to_string(payload_length));
                        do_resync();
                        return;
                    }
                    
                    do_read_payload(payload_length);
                } else {
                    Debug::log("Error reading header: " + ec.message() + ". Bytes read: " + std::to_string(length));
                    do_resync();
                }
            });
    }

    void do_resync() {
        auto self(shared_from_this());
        boost::asio::async_read(socket_, boost::asio::buffer(resync_buffer_, 1),
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    // Shift the header buffer
                    std::memmove(header_buffer_.data(), header_buffer_.data() + 1, 15);
                    header_buffer_[15] = resync_buffer_[0];
                    
                    // Check if we've found the magic number
                    uint32_t magic = (static_cast<uint32_t>(header_buffer_[0]) << 24) |
                                     (static_cast<uint32_t>(header_buffer_[1]) << 16) |
                                     (static_cast<uint32_t>(header_buffer_[2]) << 8) |
                                     static_cast<uint32_t>(header_buffer_[3]);
                    
                    if (magic == MAGIC_NUMBER) {
                        Debug::log("Resynchronized, found magic number");
                        do_read_header();
                    } else {
                        do_resync();
                    }
                } else {
                    Debug::log("Error during resync: " + ec.message());
                    do_read_header();
                }
            });
    }

    void do_read_payload(uint32_t payload_length) {
        auto self(shared_from_this());
        Debug::log("Reading payload of length " + std::to_string(payload_length));
        payload_buffer_.resize(payload_length);
        boost::asio::async_read(socket_, boost::asio::buffer(payload_buffer_),
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    Debug::log("Payload read successfully. Bytes read: " + std::to_string(length));
                    process_packet();
                } else {
                    Debug::log("Error reading payload: " + ec.message() + ". Bytes read: " + std::to_string(length));
                    do_read_header();
                }
            });
    }

    void process_packet() {
        std::vector<uint8_t> packet_data(header_buffer_.begin(), header_buffer_.end());
        packet_data.insert(packet_data.end(), payload_buffer_.begin(), payload_buffer_.end());

        try {
            Packet packet = deserializePacket(packet_data);
            Message msg = Message::deserialize({packet});
            
            Debug::log("Received message: " + msg.getContent());

            if (msg.getContent() == "RequestPeers") {
                do_write("PeerList: 127.0.0.1:6881,127.0.0.1:6882");
            } else {
                do_write("Message received");
            }
        } catch (const std::exception& e) {
            Debug::log("Error processing packet: " + std::string(e.what()));
            do_write("Error: Invalid message format");
        }

        do_read_header();  // Prepare to receive the next message
    }

    void do_write(const std::string& response) {
        auto self(shared_from_this());
        Message responseMsg("", "", response);
        std::vector<Packet> packets = responseMsg.serialize();
        
        for (const auto& packet : packets) {
            std::vector<uint8_t> serialized = serializePacket(packet);
            Debug::log("Sending response of size " + std::to_string(serialized.size()) + " bytes");
            boost::asio::async_write(socket_, boost::asio::buffer(serialized),
                [this, self](boost::system::error_code ec, std::size_t length) {
                    if (ec) {
                        Debug::log("Error writing response: " + ec.message());
                    } else {
                        Debug::log("Response sent successfully. Bytes sent: " + std::to_string(length));
                    }
                });
        }
    }

    tcp::socket socket_;
    std::array<uint8_t, 16> header_buffer_;
    std::vector<uint8_t> payload_buffer_;
    std::array<uint8_t, 1> resync_buffer_;
};

class Server {
public:
    Server(boost::asio::io_context& io_context, short port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::make_shared<Session>(std::move(socket))->start();
                }

                do_accept();
            });
    }

    tcp::acceptor acceptor_;
};

int main(int argc, char* argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "Usage: seed_node <port>\n";
            return 1;
        }

        Debug::enabled = true; // Enable debug output

        boost::asio::io_context io_context;
        Server s(io_context, std::atoi(argv[1]));
        std::cout << "Seed node running on port " << argv[1] << std::endl;
        io_context.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}