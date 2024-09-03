cmake_minimum_required(VERSION 3.10)
project(TeleLibre)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

add_executable(telelibre 
    src/main.cpp 
    # Add other source files here
)

# Example: Link with external libraries if needed
# target_link_libraries(telelibre 
#     OpenSSL::SSL 
#     OpenSSL::Crypto 
#     Boost::asio 
# )
