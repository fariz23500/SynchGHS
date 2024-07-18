#ifndef UTILS_H
#define UTILS_H

#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <iostream>
#include <algorithm> // Include this header for std::remove_if

inline void setNonBlocking(int sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "Error getting flags for socket: " << strerror(errno) << std::endl;
        return;
    }
    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "Error setting non-blocking mode for socket: " << strerror(errno) << std::endl;
    }
}

inline std::string getLocalIPAddress() {
    char buffer[128];
    std::string ipAddress = "127.0.0.1";

    FILE* fp = popen("hostname -I", "r");
    if (fp != nullptr) {
        if (fgets(buffer, sizeof(buffer), fp) != nullptr) {
            ipAddress = buffer;
            ipAddress.erase(std::remove_if(ipAddress.begin(), ipAddress.end(), ::isspace), ipAddress.end());
        }
        pclose(fp);
    }

    return ipAddress;
}

#endif // UTILS_H
