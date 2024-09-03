#ifndef PEERCONNECTION_H
#define PEERCONNECTION_H

#include <boost/asio.hpp>
#include <string>
#include <functional>
#include <memory>
#include "Message.h"

class PeerConnection : public std::enable_shared_from_this<PeerConnection> {
public:
    PeerConnection(boost::asio::io_context& io_context, 
                   const std::string& server, const std::string& port);

    void start();
    void sendMessage(const Message& msg);
    void receiveMessage();
    void setMessageHandler(std::function<void(const Message&)> handler);

    std::string getAddress() const { return server_ + ":" + port_; }

private:
    boost::asio::ip::tcp::socket socket_;
    std::string server_;
    std::string port_;
    boost::asio::streambuf receive_buffer_;
    std::function<void(const Message&)> message_handler_;
};

#endif // PEERCONNECTION_H
