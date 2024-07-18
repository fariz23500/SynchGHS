#ifndef CONFIGPARSER_H
#define CONFIGPARSER_H

#include <string>
#include <unordered_map>
#include <vector>
#include "Node.h"

struct Edge {
    unsigned int uid1;
    unsigned int uid2;
    int weight;

    Edge(unsigned int u1, unsigned int u2, int w) : uid1(u1), uid2(u2), weight(w) {}
};

void parseConfigurationFile(const std::string &filename, std::unordered_map<unsigned int, Node> &nodes, std::vector<Edge> &edges);

#endif // CONFIGPARSER_H
