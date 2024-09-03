#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <sstream>
#include "Message.h"

using boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket) : socket_(std::move(socket)) {}

    void start() {
        do_read();
    }

private:
void do_read() {
    auto self(shared_from_this());
    boost::asio::async_read_until(socket_, buffer_, "\n",
        [this, self](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                std::string serialized(boost::asio::buffers_begin(buffer_.data()),
                                       boost::asio::buffers_begin(buffer_.data()) + length - 1);
                buffer_.consume(length);

                std::cout << "Received raw: " << serialized << std::endl;
                
                try {
                    Message msg = Message::deserialize(serialized);
                    if (msg.isAcknowledgment()) {
                        std::cout << "Received acknowledgment: " << msg.content << std::endl;
                    } else {
                        std::cout << "Parsed message: ID=" << msg.message_id 
                                  << ", Group=" << msg.group_id 
                                  << ", Sender=" << msg.sender_id 
                                  << ", Content=" << msg.content << std::endl;
                        
                        // Send acknowledgment
                        do_write("Message received\n");
                    }
                } catch (const std::exception& e) {
                    std::cout << "Error parsing message: " << e.what() << std::endl;
                    do_write("Error: Invalid message format\n");
                }
            }
            do_read();
        });
}


    void do_write(std::string msg) {
        auto self(shared_from_this());
        boost::asio::async_write(socket_, boost::asio::buffer(msg),
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (ec) {
                    std::cerr << "Error writing response: " << ec.message() << std::endl;
                }
            });
    }

    tcp::socket socket_;
    boost::asio::streambuf buffer_;
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

        boost::asio::io_context io_context;

        Server s(io_context, std::atoi(argv[1]));

        std::cout << "Seed node running on port " << argv[1] << std::endl;

        io_context.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
