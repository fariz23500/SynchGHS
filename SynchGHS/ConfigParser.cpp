#include "ConfigParser.h"
#include <iostream>
#include <fstream>
#include <sstream>

void parseConfigurationFile(const std::string &filename, std::unordered_map<unsigned int, Node> &nodes, std::vector<Edge> &edges)
{
    std::ifstream configFile(filename);
    if (!configFile)
    {
        std::cerr << "Failed to open configuration file." << std::endl;
        return;
    }

    std::string line;
    int nodeCount = 0;
    bool readingNodes = true;

    std::cout << "Parsing configuration file: " << filename << std::endl;

    while (std::getline(configFile, line))
    {
        if (line.empty() || line[0] == '#')
            continue;
        std::istringstream iss(line);
        if (readingNodes)
        {
            if (!(iss >> nodeCount))
                continue;
            readingNodes = false;
            std::cout << "Number of nodes: " << nodeCount << std::endl;
        }
        else if (nodeCount > 0)
        {
            unsigned int uid;
            std::string hostname;
            unsigned short port;
            if (!(iss >> uid >> hostname >> port))
                continue;
            nodes.emplace(std::piecewise_construct,
                          std::forward_as_tuple(uid),
                          std::forward_as_tuple(uid, hostname, port));
            std::cout << "Added node: " << uid << " with hostname: " << hostname << " and port: " << port << std::endl;
            nodeCount--;
        }
        else
        {
            unsigned int uid1, uid2;
            int weight;
            char comma, parenthesis1, parenthesis2;
            if (!(iss >> parenthesis1 >> uid1 >> comma >> uid2 >> parenthesis2 >> weight))
                continue;
            edges.emplace_back(uid1, uid2, weight);
            nodes[uid1].edges.emplace_back(uid1, uid2, weight);
            nodes[uid2].edges.emplace_back(uid1, uid2, weight);
            std::cout << "Added edge: (" << uid1 << ", " << uid2 << ") with weight: " << weight << std::endl;
            std::cout << "Node " << uid1 << " edges: ";
            for (const auto &e : nodes[uid1].edges)
            {
                std::cout << "(" << e.uid1 << ", " << e.uid2 << ", " << e.weight << ") ";
            }
            std::cout << std::endl;

            std::cout << "Node " << uid2 << " edges: ";
            for (const auto &e : nodes[uid2].edges)
            {
                std::cout << "(" << e.uid1 << ", " << e.uid2 << ", " << e.weight << ") ";
            }
            std::cout << std::endl;
        }
    }

    std::cout << "Finished parsing configuration file." << std::endl;
}
