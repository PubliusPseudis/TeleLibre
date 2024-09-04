cmake_minimum_required(VERSION 3.10)
project(TeleLibre)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

# Find required packages
find_package(OpenSSL REQUIRED)
find_package(Boost COMPONENTS system REQUIRED)

# Add executables
add_executable(telelibre 
    src/main.cpp 
    src/KeyManagement.cpp
    src/Networking.cpp
    src/Message.cpp
    src/RoutingTable.cpp
    src/BloomFilter.cpp
    src/Debug.cpp
    src/Packet.cpp
)

add_executable(seed_node
    src/seed_node.cpp
    src/Message.cpp
    src/Debug.cpp
    src/Packet.cpp
)

# Link libraries
target_link_libraries(telelibre 
    OpenSSL::SSL 
    OpenSSL::Crypto 
    Boost::system
    pthread
)

target_link_libraries(seed_node
    Boost::system
    pthread
)