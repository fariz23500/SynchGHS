#ifndef NODE_H
#define NODE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <climits> // For UINT_MAX

struct EdgeInfo
{
    unsigned int uid1;
    unsigned int uid2;
    int weight;

    EdgeInfo() : uid1(0), uid2(0), weight(0) {}                                          // Default constructor
    EdgeInfo(unsigned int u1, unsigned int u2, int w) : uid1(u1), uid2(u2), weight(w) {} // Constructor with parameters
};

struct Message
{
    int round;
    std::string content;
};

struct Triplet
{
    unsigned int uid_max;
    unsigned int max_distance;
    unsigned int max_distance_uid;

    Triplet() : uid_max(0), max_distance(0), max_distance_uid(0) {}
    Triplet(unsigned int um, unsigned int md, unsigned int mdu) : uid_max(um), max_distance(md), max_distance_uid(mdu) {}
};

struct Node
{
    unsigned int UID;
    std::string hostName;
    unsigned short listeningPort;
    std::vector<unsigned int> neighbors;
    std::vector<EdgeInfo> edges;
    unsigned int mwoe;
    unsigned int UIDComponent;
    unsigned int mwoeUID1; // For the UID1 of MWOE edge
    unsigned int mwoeUID2; // For the UID2 of MWOE edge

    std::mutex mtx;
    std::condition_variable cv;
    bool ready = false;
    std::queue<Message> messageQueue;
    int currentRound = 0;
    unsigned int parent = 0;
    std::vector<unsigned int> children;

    // New members for socket handling
    int listeningSocket;
    std::vector<int> neighborSockets;

    Triplet triplet;

    Node() : UID(0), hostName(""), listeningPort(0), mwoe(UINT_MAX), UIDComponent(0), mwoeUID1(UINT_MAX), mwoeUID2(UINT_MAX), listeningSocket(-1), triplet(0, 0, 0) {} // Default constructor

    Node(unsigned int uid, const std::string &hostname, unsigned short port)
        : UID(uid), hostName(hostname), listeningPort(port), mwoe(UINT_MAX), UIDComponent(uid), mwoeUID1(UINT_MAX), mwoeUID2(UINT_MAX), listeningSocket(-1), triplet(uid, 0, 0) {}

    Node(const Node &other)
        : UID(other.UID), hostName(other.hostName), listeningPort(other.listeningPort),
          neighbors(other.neighbors), edges(other.edges), mwoe(other.mwoe), UIDComponent(other.UIDComponent), mwoeUID1(other.mwoeUID1), mwoeUID2(other.mwoeUID2), ready(other.ready),
          currentRound(other.currentRound), parent(other.parent), children(other.children),
          listeningSocket(other.listeningSocket), triplet(other.triplet) {}

    Node &operator=(const Node &other)
    {
        if (this != &other)
        {
            UID = other.UID;
            hostName = other.hostName;
            listeningPort = other.listeningPort;
            neighbors = other.neighbors;
            edges = other.edges;
            mwoe = other.mwoe;
            UIDComponent = other.UIDComponent;
            mwoeUID1 = other.mwoeUID1;
            mwoeUID2 = other.mwoeUID2;
            ready = other.ready;
            currentRound = other.currentRound;
            parent = other.parent;
            children = other.children;
            listeningSocket = other.listeningSocket;
            triplet = other.triplet;
        }
        return *this;
    }
};

#endif // NODE_H
