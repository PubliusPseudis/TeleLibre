#include "Networking.h"
#include "Debug.h"
#include "Packet.h"
#include "PeerConnection.h"
#include <iostream>
#include <boost/bind/bind.hpp>
#include <openssl/evp.h>
#include <sstream>
#include <iomanip>
#include <random>
#include <cmath>

PeerConnection::PeerConnection(boost::asio::io_context& io_context, 
                               const std::string& server, const std::string& port)
    : socket_(io_context), server_(server), port_(port) {}

void PeerConnection::start() {
    boost::asio::ip::tcp::resolver resolver(socket_.get_executor());
    auto endpoints = resolver.resolve(server_, port_);
    boost::asio::async_connect(socket_, endpoints,
        [this](boost::system::error_code ec, boost::asio::ip::tcp::endpoint) {
            if (!ec) {
                std::cout << "Connected to " << server_ << ":" << port_ << std::endl;
                receiveMessage();
            } else {
                std::cout << "Failed to connect to " << server_ << ":" << port_ << ": " << ec.message() << std::endl;
            }
        });
}

void PeerConnection::sendMessage(const Message& msg) {
    auto packets = msg.serialize();
    for (const auto& packet : packets) {
        auto serialized = serializePacket(packet);
        Debug::log("Sending packet of size " + std::to_string(serialized.size()) + " bytes");
        boost::asio::async_write(socket_, boost::asio::buffer(serialized),
            [this, self = shared_from_this()](boost::system::error_code ec, std::size_t bytes_transferred) {
                if (ec) {
                    Debug::log("Error sending message: " + ec.message());
                } else {
                    Debug::log("Successfully sent " + std::to_string(bytes_transferred) + " bytes");
                }
            });
    }
}


void PeerConnection::receiveMessage() {
    auto self(shared_from_this());
    boost::asio::async_read(socket_, receive_buffer_, boost::asio::transfer_exactly(16),
        [this, self](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                std::vector<uint8_t> header(boost::asio::buffers_begin(receive_buffer_.data()),
                                            boost::asio::buffers_begin(receive_buffer_.data()) + 16);
                receive_buffer_.consume(16);

                uint32_t payload_length = (static_cast<uint32_t>(header[4]) << 24) |
                                          (static_cast<uint32_t>(header[5]) << 16) |
                                          (static_cast<uint32_t>(header[6]) << 8) |
                                          static_cast<uint32_t>(header[7]);

                boost::asio::async_read(socket_, receive_buffer_, boost::asio::transfer_exactly(payload_length),
                    [this, self, header](boost::system::error_code ec, std::size_t length) {
                        if (!ec) {
                            std::vector<uint8_t> payload(boost::asio::buffers_begin(receive_buffer_.data()),
                                                         boost::asio::buffers_begin(receive_buffer_.data()) + length);
                            receive_buffer_.consume(length);

                            std::vector<uint8_t> packet_data = header;
                            packet_data.insert(packet_data.end(), payload.begin(), payload.end());

                            try {
                                Packet packet = deserializePacket(packet_data);
                                Message msg = Message::deserialize({packet});
                                if (message_handler_) {
                                    message_handler_(msg);
                                }
                            } catch (const std::exception& e) {
                                Debug::log("Error parsing message: " + std::string(e.what()));
                            }

                            receiveMessage();  // Continue receiving messages
                        } else {
                            Debug::log("Error receiving message payload: " + ec.message());
                        }
                    });
            } else {
                Debug::log("Error receiving message header: " + ec.message());
            }
        });
}

void PeerConnection::setMessageHandler(std::function<void(const Message&)> handler) {
    message_handler_ = handler;
}
// Update the constructor to initialize peer_update_timer_
Network::Network(boost::asio::io_context& io_context, size_t estimated_network_size)
    : io_context_(io_context), 
      bloom_filter_(estimated_network_size * 10, 5),
      estimated_network_size_(estimated_network_size),
      peer_update_timer_(io_context) {}


void Network::bootstrapNetwork(const std::vector<std::string>& seedNodes) {
    for (const auto& node : seedNodes) {
        std::string server = node.substr(0, node.find(":"));
        std::string port = node.substr(node.find(":") + 1);

        auto peer = std::make_shared<PeerConnection>(io_context_, server, port);
        peers_.push_back(peer);
        peer->start();

        peer->setMessageHandler([this](const Message& message) {
            handleIncomingMessage(message);
        });
    }

    // Wait for a short time to allow connections to be established
    boost::asio::steady_timer timer(io_context_, boost::asio::chrono::seconds(1));
    timer.async_wait([this](const boost::system::error_code&) {
        Message requestPeersMsg("", "", "RequestPeers");
        broadcastMessage(requestPeersMsg);
    });
}

void Network::sendMessage(const Message& msg) {
    if (bloom_filter_.probably_contains(msg.getMessageId())) {
        std::cout << "Message already seen, not forwarding: " << msg.getMessageId() << std::endl;
        return;
    }

    bloom_filter_.add(msg.getMessageId());
    forwardMessage(msg);
}

void Network::broadcastMessage(const Message& msg) {
    for (const auto& peer : peers_) {
        if (shouldForwardMessage()) {
            peer->sendMessage(msg);
        }
    }
}

void Network::addPeer(std::shared_ptr<PeerConnection> peer) {
    peers_.push_back(peer);
    peer->setMessageHandler([this](const Message& message) {
        handleIncomingMessage(message);
    });
}
void Network::sendPeerList() {
    std::string peerList;
    for (const auto& peer : peers_) {
        peerList += peer->getAddress() + ",";
    }
    if (!peerList.empty()) {
        peerList.pop_back(); // Remove trailing comma
    }
    Message response("", "", "PeerList: " + peerList);
    sendMessage(response);
    std::cout << "Sent peer list in response to RequestPeers" << std::endl;
}
bool Network::addPeerIfNew(const std::string& server, const std::string& port) {
    for (const auto& peer : peers_) {
        if (peer->getAddress() == server + ":" + port) {
            return false;  // Peer already exists
        }
    }
    auto newPeer = std::make_shared<PeerConnection>(io_context_, server, port);
    peers_.push_back(newPeer);
    newPeer->start();  // Start the connection for the new peer
    return true;  // Peer was added
}


void Network::updatePeerList(const std::string& peerListStr) {
    std::istringstream iss(peerListStr);
    std::string peerAddress;
    std::vector<std::shared_ptr<PeerConnection>> new_peers;

    std::lock_guard<std::mutex> lock(peers_mutex_);

    while (std::getline(iss, peerAddress, ',')) {
        std::string server = peerAddress.substr(0, peerAddress.find(":"));
        std::string port = peerAddress.substr(peerAddress.find(":") + 1);

        // Check if the peer already exists
        auto it = std::find_if(peers_.begin(), peers_.end(),
            [&](const std::shared_ptr<PeerConnection>& peer) {
                return peer->getAddress() == peerAddress;
            });

        if (it == peers_.end()) {
            auto newPeer = std::make_shared<PeerConnection>(io_context_, server, port);
            new_peers.push_back(newPeer);
        }
    }

    // Add new peers
    for (const auto& peer : new_peers) {
        peers_.push_back(peer);
        peer->start();
        peer->setMessageHandler([this](const Message& message) {
            handleIncomingMessage(message);
        });
    }

    if (!new_peers.empty()) {
        std::cout << "Added " << new_peers.size() << " new peers." << std::endl;
    }
}

void Network::startPeriodicPeerListUpdate() {
    peer_update_timer_.expires_from_now(boost::asio::chrono::minutes(5));
    peer_update_timer_.async_wait([this](const boost::system::error_code& ec) {
        if (!ec) {
            Message requestPeersMsg("", "", "RequestPeers");
            broadcastMessage(requestPeersMsg);
            startPeriodicPeerListUpdate();
        }
    });
}


void Network::handleIncomingMessage(const Message& msg) {
    if (msg.isAcknowledgment()) {
        Debug::log("Received acknowledgment: " + msg.getContent());
        return;
    }

    if (msg.getMessageId().empty()) {
        Debug::log("Received system message: " + msg.getContent());
        if (msg.getContent().substr(0, 9) == "PeerList:") {
            updatePeerList(msg.getContent().substr(10));
        } else if (msg.getContent() == "RequestPeers") {
            sendPeerList();
        }
        return;
    }

    if (msg.getContent().empty()) {
        Debug::log("Received empty message, ignoring.");
        return;
    }

    if (bloom_filter_.probably_contains(msg.getMessageId())) {
        Debug::log("Message already seen, not processing: " + msg.getMessageId());
        return;
    }

    bloom_filter_.add(msg.getMessageId());

    Debug::log("Processing message: " + msg.getContent());
    forwardMessage(msg);
    sendAcknowledgment(msg);
}



void Network::sendAcknowledgment(const Message& msg) {
    Message ack;
    ack.setContent("Message received");
    ack.setAsAcknowledgment(true);

    // Send acknowledgment to the sender
    for (const auto& peer : peers_) {
        if (peer->getAddress() == msg.getSenderId()) {
            peer->sendMessage(ack);
            break;
        }
    }
}

// Add this new method
void Network::forwardMessage(const Message& msg) {
    auto peers = routing_table_.getPeersForCategory(msg.getGroupId());
    if (!peers.empty()) {
        for (const auto& peer : peers) {
            peer->sendMessage(msg);
        }
    } else {
        int flood_radius = calculateFloodRadius();
        for (int i = 0; i < flood_radius && i < peers_.size(); ++i) {
            if (shouldForwardMessage()) {
                peers_[i]->sendMessage(msg);
            }
        }
    }
}
bool Network::shouldForwardMessage() const {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> dis(0, 1);

    const double C = 1000.0;  // Adjust this constant as needed
    return dis(gen) < (C / estimated_network_size_);
}

int Network::calculateFloodRadius() const {
    return static_cast<int>(std::ceil(std::log2(estimated_network_size_)));
}

std::string computeProofOfWork(const std::string& challenge, int difficulty) {
    int nonce = 0;
    std::string hash;

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    const EVP_MD *md = EVP_sha256();

    while (true) {
        std::stringstream ss;
        ss << challenge << nonce;
        
        unsigned char hash_raw[EVP_MAX_MD_SIZE];
        unsigned int hash_len;

        EVP_DigestInit_ex(mdctx, md, NULL);
        EVP_DigestUpdate(mdctx, ss.str().c_str(), ss.str().length());
        EVP_DigestFinal_ex(mdctx, hash_raw, &hash_len);

        std::stringstream hash_ss;
        for (unsigned int i = 0; i < hash_len; i++) {
            hash_ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash_raw[i];
        }
        hash = hash_ss.str();

        if (hash.substr(0, difficulty) == std::string(difficulty, '0')) {
            EVP_MD_CTX_free(mdctx);
            return std::to_string(nonce);
        }

        nonce++;
    }
}
