#ifndef ROUTINGTABLE_H
#define ROUTINGTABLE_H

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include "PeerConnection.h"

class RoutingTable {
public:
    void addPeer(const std::string& category, std::shared_ptr<PeerConnection> peer);
    std::vector<std::shared_ptr<PeerConnection>> getPeersForCategory(const std::string& category);
    void updatePeerInterests(std::shared_ptr<PeerConnection> peer, const std::vector<std::string>& categories);

private:
    std::unordered_map<std::string, std::vector<std::shared_ptr<PeerConnection>>> table_;
};

#endif // ROUTINGTABLE_H
