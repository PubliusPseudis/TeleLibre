#include "Networking.h"
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
    auto self(shared_from_this());
    std::string serialized = msg.is_acknowledgment ? msg.content : msg.serialize();
    boost::asio::async_write(socket_, boost::asio::buffer(serialized + "\n"),
        [this, self, serialized](boost::system::error_code ec, std::size_t) {
            if (ec) {
                std::cout << "Error sending message to " << server_ << ":" << port_ << ": " << ec.message() << std::endl;
            } else {
                std::cout << "Message sent to " << server_ << ":" << port_ << ": " << serialized << std::endl;
            }
        });
}

void PeerConnection::receiveMessage() {
    auto self(shared_from_this());
    boost::asio::async_read_until(socket_, receive_buffer_, "\n",
        [this, self](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                std::string serialized;
                std::istream is(&receive_buffer_);
                std::getline(is, serialized);
                std::cout << "Received from " << server_ << ":" << port_ << ": " << serialized << std::endl;

                try {
                    Message msg = Message::deserialize(serialized);
                    if (msg.message_id.empty()) {
                        // This is a system message
                        std::cout << "Received system message: " << msg.content << std::endl;
                    } else {
                        std::cout << "Received message: ID=" << msg.message_id 
                                  << ", Group=" << msg.group_id 
                                  << ", Sender=" << msg.sender_id 
                                  << ", Content=" << msg.content << std::endl;
                    }
                    if (message_handler_) {
                        message_handler_(msg);
                    }
                } catch (const std::exception& e) {
                    std::cout << "Error parsing message from " << server_ << ":" << port_ << ": " << e.what() << std::endl;
                }

                receiveMessage();
            } else {
                std::cout << "Error receiving message from " << server_ << ":" << port_ << ": " << ec.message() << std::endl;
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
    if (bloom_filter_.probably_contains(msg.message_id)) {
        std::cout << "Message already seen, not forwarding: " << msg.message_id << std::endl;
        return;
    }

    bloom_filter_.add(msg.message_id);
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
    if (msg.is_acknowledgment) {
        std::cout << "Received acknowledgment: " << msg.content << std::endl;
        return;  // Don't process or forward acknowledgments
    }

    if (msg.content.empty()) {
        std::cout << "Received empty message, ignoring." << std::endl;
        return;
    }

    // Handle regular messages
    if (bloom_filter_.probably_contains(msg.message_id)) {
        std::cout << "Message already seen, not processing: " << msg.message_id << std::endl;
        return;
    }

    bloom_filter_.add(msg.message_id);

    std::cout << "Processing message: " << msg.content << std::endl;
    // Forward the message
    forwardMessage(msg);

    // Send acknowledgment
    sendAcknowledgment(msg);
}
void Network::sendAcknowledgment(const Message& msg) {
    Message ack;
    ack.content = "Message received";
    ack.is_acknowledgment = true;

    // Send acknowledgment to the sender
    for (const auto& peer : peers_) {
        if (peer->getAddress() == msg.sender_id) {
            peer->sendMessage(ack);
            break;
        }
    }
}

// Add this new method
void Network::forwardMessage(const Message& msg) {
    auto peers = routing_table_.getPeersForCategory(msg.group_id);
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
