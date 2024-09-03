# TeleLibre: A Fully Decentralized Meme Sharing Protocol

## Overview

TeleLibre is a fully decentralized protocol designed for real-time sharing of memes across a peer-to-peer network. This project aims to create a robust, censorship-resistant platform where all nodes are equal, ensuring true decentralization with no central servers or special nodes.

### Key Features

- **True Decentralization**: Every node acts as both client and server, ensuring there are no single points of failure.
- **Scalability**: Designed to scale efficiently with logarithmic performance characteristics.
- **Resilience**: Handles network churn and partitions gracefully, ensuring consistent and reliable communication.
- **Security**: Built-in measures to prevent common attacks, such as Sybil attacks, with optional content moderation frameworks.

## Getting Started

### Prerequisites

- C++17 or later
- CMake 3.10 or later
- Boost.Asio
- OpenSSL
- LevelDB (for data persistence)

### Installation

1. **Clone the Repository**
   ```bash
   git clone https://github.com/yourusername/telelibre.git
   cd telelibre
Build the Project

bash

mkdir build
cd build
cmake ..
make

Run TeleLibre

bash

    ./telelibre

Usage
Basic Commands

    Create a Group:

    bash

./telelibre create_group "Meme Enthusiasts"

Join a Group:

bash

./telelibre join_group <group_id>

Send a Message:

bash

    ./telelibre send_message <group_id> "Check out this meme!"

Configuration

You can customize various settings, such as storage paths and network configurations, by modifying the config.json file in the project root.
Documentation

For detailed technical specifications, please refer to the TeleLibre Technical Specification.
Contributing

We welcome contributions from the community.

