#include "Node.h"
#include "Utils.h"
#include <iostream>
#include <unordered_map>
#include <vector>
#include <unordered_set>
#include <string>
#include <thread>
#include <chrono>
#include <algorithm>
#include <climits>
#include <arpa/inet.h>
#include <netinet/sctp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdexcept>

extern std::unordered_map<unsigned int, Node> nodes;

void broadcastMessage(Node &node, const std::string &message)
{
    for (int sock : node.neighborSockets)
    {
        if (send(sock, message.c_str(), message.size(), 0) == -1)
        {
            std::cerr << "Error sending message from node " << node.UID << " to socket " << sock << ": " << strerror(errno) << std::endl;
        }
        else
        {
            std::cout << "Node " << node.UID << " sent message to socket " << sock << ": " << message << std::endl;
        }
    }
}

std::string receiveMessage(Node &node)
{
    char buffer[1024];
    std::string result;
    for (int sock : node.neighborSockets)
    {
        int bytesRead = recv(sock, buffer, sizeof(buffer), 0);
        if (bytesRead > 0)
        {
            result += std::string(buffer, bytesRead) + " ";
            std::cout << "Node " << node.UID << " received message from socket " << sock << ": " << std::string(buffer, bytesRead) << std::endl;
        }
        else if (bytesRead == 0)
        {
            std::cerr << "Connection closed by peer on socket " << sock << std::endl;
            close(sock);
        }
        else if (errno != EWOULDBLOCK && errno != EAGAIN)
        {
            std::cerr << "Error receiving message on node " << node.UID << " from socket " << sock << ": " << strerror(errno) << std::endl;
        }
    }
    return result;
}

void findMWOE(Node &node)
{
    unsigned int mwoe = UINT_MAX;
    for (const auto &edge : node.edges)
    {
        std::cout << "Node " << node.UID << " checking edge (" << edge.uid1 << ", " << edge.uid2 << ") with weight " << edge.weight << std::endl;
        if (nodes[edge.uid1].UIDComponent != nodes[edge.uid2].UIDComponent)
        {
            mwoe = std::min(mwoe, static_cast<unsigned int>(edge.weight));
        }
    }
    node.mwoe = mwoe;
    std::cout << "Node " << node.UID << " found MWOE: " << node.mwoe << std::endl;
}

void updateComponent(unsigned int newComponentID, unsigned int oldComponentID)
{
    for (auto &pair : nodes)
    {
        if (pair.second.UIDComponent == oldComponentID)
        {
            pair.second.UIDComponent = newComponentID;
        }
    }
}

bool processMWOE(Node &node, const std::string &msg)
{
    unsigned int minMWOE = UINT_MAX;
    bool mergeOccurred = false;
    try
    {
        for (const auto &edge : node.edges)
        {
            if (edge.weight == node.mwoe)
            {
                unsigned int otherNode = (edge.uid1 == node.UID) ? edge.uid2 : edge.uid1;
                if (nodes[edge.uid1].UIDComponent != nodes[edge.uid2].UIDComponent)
                {
                    unsigned int newComponentID = std::max(nodes[edge.uid1].UIDComponent, nodes[edge.uid2].UIDComponent);
                    unsigned int oldComponentID = nodes[edge.uid1].UIDComponent == newComponentID ? nodes[edge.uid2].UIDComponent : nodes[edge.uid1].UIDComponent;

                    updateComponent(newComponentID, oldComponentID);

                    nodes[edge.uid1].UIDComponent = newComponentID;
                    nodes[edge.uid2].UIDComponent = newComponentID;

                    if (node.UID == edge.uid1)
                    {
                        node.children.push_back(edge.uid2);
                    }
                    else
                    {
                        node.children.push_back(edge.uid1);
                    }

                    nodes[otherNode].parent = node.UID;
                    mergeOccurred = true;
                    if (!msg.empty())
                    {
                        minMWOE = std::min(minMWOE, static_cast<unsigned int>(std::stoul(msg)));
                    }
                }
            }
        }
    }
    catch (const std::invalid_argument &e)
    {
        std::cerr << "Invalid argument error: " << e.what() << " while processing MWOE for node " << node.UID << std::endl;
    }
    catch (const std::out_of_range &e)
    {
        std::cerr << "Out of range error: " << e.what() << " while processing MWOE for node " << node.UID << std::endl;
    }
    node.mwoe = minMWOE;
    std::cout << "Node " << node.UID << " processed MWOE: " << node.mwoe << std::endl;
    return mergeOccurred;
}

void reorientTree(unsigned int leaderUID)
{
    std::unordered_map<unsigned int, bool> visited;
    std::queue<unsigned int> q;
    q.push(leaderUID);
    visited[leaderUID] = true;

    while (!q.empty())
    {
        unsigned int currentUID = q.front();
        q.pop();
        Node &currentNode = nodes[currentUID];

        for (unsigned int childUID : currentNode.children)
        {
            if (!visited[childUID])
            {
                visited[childUID] = true;
                nodes[childUID].parent = currentUID;
                q.push(childUID);
            }
        }
    }
}

void buildTreeFromRoot(unsigned int rootUID)
{
    // Step 1: Create an adjacency list from the current nodes
    std::unordered_map<unsigned int, std::unordered_set<unsigned int>> adjacencyList;

    for (const auto &pair : nodes)
    {
        const Node &node = pair.second;
        for (unsigned int child : node.children)
        {
            adjacencyList[node.UID].insert(child);
            adjacencyList[child].insert(node.UID);
        }
        if (node.parent != 0)
        {
            adjacencyList[node.UID].insert(node.parent);
            adjacencyList[node.parent].insert(node.UID);
        }
    }

    // Step 2: Perform BFS to build the tree from the rootUID
    std::unordered_map<unsigned int, bool> visited;
    std::queue<unsigned int> q;
    q.push(rootUID);
    visited[rootUID] = true;

    // Set the root node parent to 0 (or null equivalent)
    nodes[rootUID].parent = 0;
    nodes[rootUID].children.clear();

    while (!q.empty())
    {
        unsigned int currentUID = q.front();
        q.pop();
        Node &currentNode = nodes[currentUID];

        for (unsigned int neighborUID : adjacencyList[currentUID])
        {
            if (!visited[neighborUID])
            {
                visited[neighborUID] = true;
                nodes[neighborUID].parent = currentUID;
                nodes[neighborUID].children.clear();
                currentNode.children.push_back(neighborUID);
                q.push(neighborUID);
            }
        }
    }

    // Rebuild the adjacency list with the correct orientation
    for (auto &pair : nodes)
    {
        Node &node = pair.second;
        std::vector<unsigned int> newChildren;
        for (unsigned int childUID : node.children)
        {
            if (nodes[childUID].parent == node.UID)
            {
                newChildren.push_back(childUID);
            }
        }
        node.children = newChildren;
    }
}

void runSyncGHS()
{
    while (true)
    {
        bool mergeOccurred = false;

        for (auto &pair : nodes)
        {
            Node &node = pair.second;
            findMWOE(node);
            if (!node.messageQueue.empty())
            {
                broadcastMessage(node, std::to_string(node.messageQueue.front().round) + " " + node.messageQueue.front().content);
            }
            else
            {
                broadcastMessage(node, std::to_string(node.currentRound) + " MWOE:" + std::to_string(node.mwoe));
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(1)); // Ensure all nodes have time to send their messages

        for (auto &pair : nodes)
        {
            Node &node = pair.second;
            std::string messages = receiveMessage(node);
            if (processMWOE(node, messages))
            {
                mergeOccurred = true;
            }
        }

        // Determine the new leader (maximum UID of core edge)
        unsigned int newLeaderUID = 0;
        for (const auto &pair : nodes)
        {
            if (pair.second.UIDComponent > newLeaderUID)
            {
                newLeaderUID = pair.second.UIDComponent;
            }
        }

        // Update component IDs and reorient the tree
        for (auto &pair : nodes)
        {
            if (pair.second.UIDComponent == newLeaderUID)
            {
                updateComponent(newLeaderUID, pair.second.UIDComponent);
            }
        }

        reorientTree(newLeaderUID);

        // Check for termination condition
        bool allIntMax = true;
        for (const auto &pair : nodes)
        {
            if (pair.second.mwoe != UINT_MAX)
            {
                allIntMax = false;
                break;
            }
        }
        if (allIntMax && !mergeOccurred)
        {
            break;
        }
    }

    unsigned int rootUID = 0;
    for (const auto &pair : nodes)
    {
        if (pair.second.parent == 0 && pair.first > rootUID)
        {
            rootUID = pair.first;
        }
    }
    // std::cout << "Final BFS Tree Structure:\n";
    // for (const auto &pair : nodes)
    // {
    //     const Node &node = pair.second;
    //     std::cout << "Node " << node.UID << " - Parent: " << node.parent << ", Children: ";
    //     for (unsigned int child : node.children)
    //     {
    //         std::cout << child << " ";
    //     }
    //     std::cout << std::endl;
    // }

    // Reorient the tree around the new root
    buildTreeFromRoot(78);

    // Print final tree structure
    std::cout << "reoriented Final BFS Tree Structure:\n";
    for (const auto &pair : nodes)
    {
        const Node &node = pair.second;
        std::cout << "Node " << node.UID << " - Parent: " << node.parent << ", Children: ";
        for (unsigned int child : node.children)
        {
            std::cout << child << " ";
        }
        std::cout << std::endl;
    }
}
