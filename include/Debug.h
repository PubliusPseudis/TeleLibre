#ifndef DEBUG_H
#define DEBUG_H

#include <iostream>
#include <string>

class Debug {
public:
    static bool enabled;

    static void log(const std::string& message) {
        if (enabled) {
            std::cout << "[DEBUG] " << message << std::endl;
        }
    }
};

#endif // DEBUG_H