// MainCoordinator.cpp
#include "ConfigParser.h"
#include "Node.h"
#include <iostream>
#include <thread>
#include <vector>
#include <unordered_map>
#include <csignal>
#include <unistd.h>

std::unordered_map<unsigned int, Node> nodes;
std::vector<Edge> edges;

void parseConfigurationFile(const std::string &filename, std::unordered_map<unsigned int, Node> &nodes, std::vector<Edge> &edges);
void connectNodes();
void runSyncGHS();

void cleanup(int signum) {
    std::cout << "Cleaning up sockets..." << std::endl;
    for (auto &pair : nodes) {
        Node &node = pair.second;
        for (int sock : node.neighborSockets) {
            close(sock);
        }
    }
    exit(signum);
}

int main() {
    signal(SIGINT, cleanup); // Register signal handler for cleanup on exit

    std::cout << "Parsing configuration file..." << std::endl;
    parseConfigurationFile("config.txt", nodes, edges);

    std::this_thread::sleep_for(std::chrono::seconds(5));

    std::cout << "Connecting nodes..." << std::endl;
    connectNodes();

    std::this_thread::sleep_for(std::chrono::seconds(5));

    std::cout << "Running SyncGHS..." << std::endl;
    runSyncGHS();

    return 0;
}
