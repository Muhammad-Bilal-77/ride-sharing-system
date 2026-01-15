#ifndef CITY_H
#define CITY_H

#include <iostream>
#include <fstream>
#include <cstring>
#include <cmath>

const int MAX_STRING_LENGTH = 256;

// Represents a node in the city graph
struct Node
{
    char id[MAX_STRING_LENGTH];
    char zone[MAX_STRING_LENGTH];
    char colony[MAX_STRING_LENGTH];
    char street[MAX_STRING_LENGTH];
    int streetNo;
    int nodeNo;
    double x;                             // X coordinate in meters
    double y;                             // Y coordinate in meters
    char locationType[MAX_STRING_LENGTH]; // "street", "home", "hospital", "school", "mall", etc.
    char locationName[MAX_STRING_LENGTH]; // Name if it's a location

    Node *next; // For linked list

    Node();
};

// Represents an edge in adjacency list
struct EdgeNode
{
    char toNodeId[MAX_STRING_LENGTH];
    double weight; // Distance in meters
    char connectionType[MAX_STRING_LENGTH];
    EdgeNode *next;

    EdgeNode();
};

// Adjacency list entry (one per node that has edges)
struct AdjListNode
{
    char nodeId[MAX_STRING_LENGTH];
    EdgeNode *edges;   // Linked list of edges
    AdjListNode *next; // Next adjacency list entry

    AdjListNode();
};

class City
{
private:
    Node *nodeListHead;             // Dynamic linked list of all nodes
    AdjListNode *adjacencyListHead; // Dynamic linked list for adjacency entries
    int nodeCount;
    int edgeCount;

    // Helper methods
    void trim(char *str) const;
    void removeQuotes(char *str) const;
    bool parseCsvLine(const char *line, char fields[][MAX_STRING_LENGTH], int &fieldCount) const;
    double calculateDistance(double x1, double y1, double x2, double y2) const;

    // Node management - grows dynamically
    void insertNode(Node *node);
    void addEdge(const char *fromId, const char *toId, double weight, const char *connType);
    EdgeNode *getEdges(const char *nodeId) const;
    AdjListNode *findOrCreateAdjListNode(const char *nodeId);

public:
    City();
    ~City();

    // Load data from CSV files
    bool loadLocations(const char *filePath);
    bool loadPaths(const char *filePath);

    // Query methods
    Node *getNode(const char *nodeId) const;
    void getNodesByType(const char *locationType, Node *results[], int &count, int maxResults) const;
    EdgeNode *getNeighbors(const char *nodeId) const;

    // Utility methods
    double getDistance(const char *nodeId1, const char *nodeId2) const;
    Node *findNearestNode(double x, double y) const;

    // Statistics
    int getNodeCount() const;
    int getEdgeCount() const;

    // Display helper
    void printNodeInfo(const Node *node) const;

    // Iterator helper (to traverse nodes)
    Node *getFirstNode() const;
};

#endif // CITY_H
