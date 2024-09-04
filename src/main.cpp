#include <iostream>
#include <string>
#include <cstring>
#include <chrono>
#include <thread>
#include "KeyManagement.h"
#include "Networking.h"
#include "Message.h"
#include "Debug.h"

void runKeyManagementTest() {
    std::cout << "\n--- Key Management Test ---\n";
    EVP_PKEY *privateKey = nullptr;
    EVP_PKEY *publicKey = nullptr;
    unsigned char *signature = nullptr;

    try {
        KeyManagement::generateKeys(&privateKey, &publicKey);
        std::cout << "Keys generated successfully." << std::endl;

        KeyManagement::savePrivateKey(privateKey, "private.pem");
        KeyManagement::savePublicKey(publicKey, "public.pem");
        std::cout << "Keys saved to files." << std::endl;

        const char *message_content = "Hello, TeleLibre!";
        size_t signatureLen = 0;

        if (KeyManagement::signMessage(privateKey, reinterpret_cast<const unsigned char*>(message_content), 
                                       strlen(message_content), &signature, &signatureLen) == 1) {
            std::cout << "Message signed successfully." << std::endl;

            if (KeyManagement::verifyMessage(publicKey, reinterpret_cast<const unsigned char*>(message_content), 
                                             strlen(message_content), signature, signatureLen) == 1) {
                std::cout << "Message verified successfully." << std::endl;
            } else {
                std::cout << "Message verification failed." << std::endl;
            }
        } else {
            std::cout << "Message signing failed." << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error in key management test: " << e.what() << std::endl;
    }

    if (signature) OPENSSL_free(signature);
    if (privateKey) EVP_PKEY_free(privateKey);
    if (publicKey) EVP_PKEY_free(publicKey);
}

void runNetworkingTest(boost::asio::io_context& io_context) {
    std::cout << "\n--- Networking Test ---\n";
    try {
        Debug::log("--- Networking Test ---");
        boost::asio::io_context io_context;
        Network network(io_context, 1000);  // Assume an estimated network size of 1000 nodes

        std::vector<std::string> seedNodes = {"127.0.0.1:6881", "127.0.0.1:6882"};
        Debug::log("Bootstrapping network with seed nodes: " + seedNodes[0] + ", " + seedNodes[1]);
        network.bootstrapNetwork(seedNodes);

        // Send test messages with delays
        Message testMsg1("test_group", "test_sender", "This is a test message");
        Debug::log("Sending test message 1");
        network.sendMessage(testMsg1);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        Message testMsg2("test_group", "test_sender", "This is a second test message");
        Debug::log("Sending test message 2");
        network.sendMessage(testMsg2);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        Message requestPeersMsg("", "", "RequestPeers");
        Debug::log("Sending RequestPeers message");
        network.sendMessage(requestPeersMsg);

        // Run the io_context for a short time to allow for message processing
        boost::asio::steady_timer timer(io_context, boost::asio::chrono::seconds(5));
        timer.async_wait([&io_context](const boost::system::error_code&) { 
            Debug::log("Stopping io_context after 5 seconds");
            io_context.stop(); 
        });

        Debug::log("Running io_context");
        io_context.run();

    } catch (const std::exception& e) {
        std::cerr << "Error in networking test: " << e.what() << std::endl;
    }
}

void runProofOfWorkTest() {
    std::cout << "\n--- Proof of Work Test ---\n";
    std::string challenge = "TeleLibreChallenge";
    int difficulty = 4;
    std::cout << "Starting Proof of Work with difficulty " << difficulty << std::endl;
    std::string nonce = computeProofOfWork(challenge, difficulty);
    std::cout << "Proof of Work completed. Nonce: " << nonce << std::endl;
}

int main() {
    Debug::enabled = true; // Enable debug output

    std::cout << "TeleLibre: Decentralized Meme Sharing Protocol" << std::endl;

    runKeyManagementTest();

    boost::asio::io_context io_context;
    runNetworkingTest(io_context);

    runProofOfWorkTest();

    return 0;
}
