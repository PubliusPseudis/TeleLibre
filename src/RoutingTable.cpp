#include "RoutingTable.h"
#include <algorithm>

void RoutingTable::addPeer(const std::string& category, std::shared_ptr<PeerConnection> peer) {
    table_[category].push_back(peer);
}

std::vector<std::shared_ptr<PeerConnection>> RoutingTable::getPeersForCategory(const std::string& category) {
    if (table_.find(category) != table_.end()) {
        return table_[category];
    }
    return {};
}

void RoutingTable::updatePeerInterests(std::shared_ptr<PeerConnection> peer, const std::vector<std::string>& categories) {
    // Remove the peer from all categories
    for (auto& entry : table_) {
        auto& peers = entry.second;
        peers.erase(std::remove(peers.begin(), peers.end(), peer), peers.end());
    }

    // Add the peer to the specified categories
    for (const auto& category : categories) {
        addPeer(category, peer);
    }
}
