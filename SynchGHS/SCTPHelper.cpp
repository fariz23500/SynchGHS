#include "Node.h"
#include "Utils.h"
#include <iostream>
#include <unordered_map>
#include <vector>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/sctp.h>

extern std::unordered_map<unsigned int, Node> nodes;

void initSockets()
{
    for (auto &pair : nodes)
    {
        Node &node = pair.second;

        node.listeningSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (node.listeningSocket < 0)
        {
            std::cerr << "Error creating socket for node " << node.UID << std::endl;
            continue;
        }

        setNonBlocking(node.listeningSocket);

        int opt = 1;
        if (setsockopt(node.listeningSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        {
            std::cerr << "Error setting socket options for node " << node.UID << std::endl;
            close(node.listeningSocket);
            continue;
        }

        sockaddr_in address;
        address.sin_family = AF_INET;
        inet_pton(AF_INET, node.hostName.c_str(), &address.sin_addr); // Bind to specific IP address
        address.sin_port = htons(node.listeningPort);

        if (bind(node.listeningSocket, (struct sockaddr *)&address, sizeof(address)) < 0)
        {
            std::cerr << "Error binding socket for node " << node.UID << " on port " << node.listeningPort
                      << ": " << strerror(errno) << std::endl;
            close(node.listeningSocket);
            continue;
        }

        if (listen(node.listeningSocket, 5) < 0)
        {
            std::cerr << "Error listening on socket for node " << node.UID << std::endl;
            close(node.listeningSocket);
            continue;
        }

        std::cout << "Node " << node.UID << " listening on port " << node.listeningPort << " with IP " << node.hostName << std::endl;
    }
}

void startServer(unsigned int uid)
{
    Node &node = nodes.at(uid);

    while (true)
    {
        sockaddr_in clientAddress;
        socklen_t clientAddressLen = sizeof(clientAddress);
        int newSocket = accept(node.listeningSocket, (struct sockaddr *)&clientAddress, &clientAddressLen);

        if (newSocket < 0)
        {
            if (errno != EWOULDBLOCK && errno != EAGAIN)
            {
                std::cerr << "Error accepting connection on node " << node.UID << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        setNonBlocking(newSocket);
        node.neighborSockets.push_back(newSocket);
        std::cout << "Node " << node.UID << " accepted connection on socket " << newSocket << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
}

void connectNodes()
{
    for (auto &pair : nodes)
    {
        Node &node = pair.second;

        std::cout << "Node " << node.UID << " has edges: ";
        for (const EdgeInfo &edge : node.edges)
        {
            std::cout << "(" << edge.uid1 << ", " << edge.uid2 << ", " << edge.weight << ") ";
        }
        std::cout << std::endl;

        for (const EdgeInfo &edge : node.edges)
        {
            unsigned int neighborUID = (node.UID == edge.uid1) ? edge.uid2 : edge.uid1;
            Node &neighbor = nodes.at(neighborUID);

            sockaddr_in address;
            address.sin_family = AF_INET;
            inet_pton(AF_INET, neighbor.hostName.c_str(), &address.sin_addr);
            address.sin_port = htons(neighbor.listeningPort);

            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0)
            {
                std::cerr << "Error creating socket for node " << node.UID << " to connect to neighbor " << neighborUID << std::endl;
                continue;
            }

            setNonBlocking(sock);

            while (connect(sock, (struct sockaddr *)&address, sizeof(address)) < 0)
            {
                if (errno == EINPROGRESS)
                {
                    std::cout << "Node " << node.UID << " attempting to connect to neighbor " << neighborUID << std::endl;
                    std::this_thread::sleep_for(std::chrono::seconds(1)); // Added sleep to give time for the connection
                }
                else
                {
                    std::cerr << "Error connecting node " << node.UID << " to neighbor " << neighborUID << ": " << strerror(errno) << std::endl;
                    close(sock);
                    break;
                }
            }

            node.neighborSockets.push_back(sock);
            std::cout << "Node " << node.UID << " successfully connected to neighbor " << neighborUID << " with IP " << neighbor.hostName << std::endl;
        }
    }
}
