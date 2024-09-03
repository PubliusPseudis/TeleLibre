#ifndef NETWORKING_H
#define NETWORKING_H

#include <boost/asio.hpp>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include "Message.h"
#include "RoutingTable.h"
#include "BloomFilter.h"
#include "PeerConnection.h"

class Network {
public:
    Network(boost::asio::io_context& io_context, size_t estimated_network_size);
    void bootstrapNetwork(const std::vector<std::string>& seedNodes);
    void sendMessage(const Message& msg);
    void broadcastMessage(const Message& msg);
    void addPeer(std::shared_ptr<PeerConnection> peer);
    void updatePeerList(const std::string& peerListStr);
    void startPeriodicPeerListUpdate();
    
private:
    boost::asio::io_context& io_context_;
    RoutingTable routing_table_;
    std::vector<std::shared_ptr<PeerConnection>> peers_;
    BloomFilter bloom_filter_;
    size_t estimated_network_size_;
    boost::asio::steady_timer peer_update_timer_;
    std::mutex peers_mutex_;

    void handleIncomingMessage(const Message& msg);
    void sendAcknowledgment(const Message &msg);
    bool addPeerIfNew(const std::string &server, const std::string &port);
    void sendPeerList();
    bool shouldForwardMessage() const;
    int calculateFloodRadius() const;
    void forwardMessage(const Message& msg);
};

std::string computeProofOfWork(const std::string& challenge, int difficulty);

#endif // NETWORKING_H
