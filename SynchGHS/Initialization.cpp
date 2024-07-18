#include <iostream>
#include <thread>
#include <vector>
#include <unordered_map>
#include <csignal>
#include <unistd.h>
#include "Node.h"
#include <algorithm>
#include "ConfigParser.h"

std::unordered_map<unsigned int, Node> nodes;
std::vector<Edge> edges;

void parseConfigurationFile(const std::string &filename);
void initSockets();
void startServer(unsigned int uid);
void cleanup(int signum);

std::string getLocalIPAddress()
{
    // This function should return the local IP address of the machine
    char buffer[128];
    std::string ipAddress = "10.176.69.32"; // default to localhost
    FILE *pipe = popen("hostname -I", "r");
    if (pipe)
    {
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        {
            ipAddress = buffer;
            break;
        }
        pclose(pipe);
    }
    // Trim the IP address to remove any trailing spaces or newlines
    ipAddress.erase(std::remove_if(ipAddress.begin(), ipAddress.end(), ::isspace), ipAddress.end());
    return ipAddress;
}

int main()
{
    signal(SIGINT, cleanup); // Register signal handler for cleanup on exit

    std::cout << "Parsing configuration file..." << std::endl;
    parseConfigurationFile("config.txt", nodes, edges);
    std::string localIPAddress = getLocalIPAddress();
    std::cout << localIPAddress << " ";
    std::cout << "Initializing sockets..." << std::endl;
    initSockets();

    std::cout << "Starting servers..." << std::endl;
    std::vector<std::thread> serverThreads;

    for (const auto &pair : nodes)
    {
        if (pair.second.hostName == localIPAddress)
        {
            serverThreads.push_back(std::thread(startServer, pair.second.UID));
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
    }

    // Wait for servers to run indefinitely
    for (auto &thread : serverThreads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    return 0;
}

void cleanup(int signum)
{
    std::cout << "Cleaning up sockets..." << std::endl;
    for (auto &pair : nodes)
    {
        Node &node = pair.second;
        if (node.listeningSocket != -1)
        {
            close(node.listeningSocket);
        }
        for (int sock : node.neighborSockets)
        {
            close(sock);
        }
    }
    exit(signum);
}