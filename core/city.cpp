#include "city.h"
#include <cstring>
#include <fstream>
#include <iostream>

// Node constructor
Node::Node() : streetNo(0), nodeNo(0), x(0.0), y(0.0), next(nullptr)
{
    id[0] = '\0';
    zone[0] = '\0';
    colony[0] = '\0';
    street[0] = '\0';
    locationType[0] = '\0';
    locationName[0] = '\0';
}

// EdgeNode constructor
EdgeNode::EdgeNode() : weight(0.0), next(nullptr)
{
    toNodeId[0] = '\0';
    connectionType[0] = '\0';
}

// AdjListNode constructor
AdjListNode::AdjListNode() : edges(nullptr), next(nullptr)
{
    nodeId[0] = '\0';
}

// City constructor
City::City() : nodeListHead(nullptr), adjacencyListHead(nullptr), nodeCount(0), edgeCount(0)
{
}

// City destructor - clean up all dynamically allocated memory
City::~City()
{
    // Clean up node linked list
    Node *currentNode = nodeListHead;
    while (currentNode != nullptr)
    {
        Node *temp = currentNode;
        currentNode = currentNode->next;
        delete temp;
    }

    // Clean up adjacency list
    AdjListNode *currentAdj = adjacencyListHead;
    while (currentAdj != nullptr)
    {
        // Clean up edges for this node
        EdgeNode *edge = currentAdj->edges;
        while (edge != nullptr)
        {
            EdgeNode *tempEdge = edge;
            edge = edge->next;
            delete tempEdge;
        }
        AdjListNode *temp = currentAdj;
        currentAdj = currentAdj->next;
        delete temp;
    }
}

// Trim whitespace from string
void City::trim(char *str) const
{
    if (!str || str[0] == '\0')
        return;

    // Trim leading spaces
    int i = 0;
    while (str[i] == ' ' || str[i] == '\t' || str[i] == '\r' || str[i] == '\n')
    {
        i++;
    }
    if (i > 0)
    {
        int j = 0;
        while (str[i])
        {
            str[j++] = str[i++];
        }
        str[j] = '\0';
    }

    // Trim trailing spaces
    int len = strlen(str);
    while (len > 0 && (str[len - 1] == ' ' || str[len - 1] == '\t' || str[len - 1] == '\r' || str[len - 1] == '\n'))
    {
        str[--len] = '\0';
    }
}

// Remove quotes from string
void City::removeQuotes(char *str) const
{
    if (!str || str[0] == '\0')
        return;

    trim(str);
    int len = strlen(str);
    if (len >= 2 && str[0] == '"' && str[len - 1] == '"')
    {
        for (int i = 0; i < len - 2; i++)
        {
            str[i] = str[i + 1];
        }
        str[len - 2] = '\0';
    }
}

// Parse CSV line handling quoted fields
bool City::parseCsvLine(const char *line, char fields[][MAX_STRING_LENGTH], int &fieldCount) const
{
    fieldCount = 0;
    int fieldIndex = 0;
    int charIndex = 0;
    bool inQuotes = false;

    for (int i = 0; line[i] != '\0'; i++)
    {
        char c = line[i];

        if (c == '"')
        {
            inQuotes = !inQuotes;
        }
        else if (c == ',' && !inQuotes)
        {
            fields[fieldCount][charIndex] = '\0';
            fieldCount++;
            charIndex = 0;
            if (fieldCount >= 30)
                break; // Safety limit
        }
        else
        {
            if (charIndex < MAX_STRING_LENGTH - 1)
            {
                fields[fieldCount][charIndex++] = c;
            }
        }
    }
    fields[fieldCount][charIndex] = '\0';
    fieldCount++;

    return fieldCount > 0;
}

// Calculate Euclidean distance
double City::calculateDistance(double x1, double y1, double x2, double y2) const
{
    double dx = x2 - x1;
    double dy = y2 - y1;
    return sqrt(dx * dx + dy * dy);
}

// Insert node into dynamic linked list (grows automatically)
void City::insertNode(Node *node)
{
    node->next = nodeListHead;
    nodeListHead = node;
    nodeCount++;
}

// Find or create adjacency list node (grows dynamically)
AdjListNode *City::findOrCreateAdjListNode(const char *nodeId)
{
    // Search for existing entry
    AdjListNode *current = adjacencyListHead;
    while (current != nullptr)
    {
        if (strcmp(current->nodeId, nodeId) == 0)
        {
            return current;
        }
        current = current->next;
    }

    // Create new entry and add to head (list grows dynamically)
    AdjListNode *newNode = new AdjListNode();
    strcpy(newNode->nodeId, nodeId);
    newNode->next = adjacencyListHead;
    adjacencyListHead = newNode;
    return newNode;
}

// Add undirected edge (adds both directions, grows dynamically)
void City::addEdge(const char *fromId, const char *toId, double weight, const char *connType)
{
    // Add edge from -> to if it does not already exist
    AdjListNode *fromNode = findOrCreateAdjListNode(fromId);
    EdgeNode *edgeWalk = fromNode->edges;
    while (edgeWalk != nullptr)
    {
        if (strcmp(edgeWalk->toNodeId, toId) == 0)
        {
            // Edge already present; skip duplicate
            return;
        }
        edgeWalk = edgeWalk->next;
    }

    EdgeNode *newEdge = new EdgeNode();
    strcpy(newEdge->toNodeId, toId);
    newEdge->weight = weight;
    strcpy(newEdge->connectionType, connType);
    newEdge->next = fromNode->edges;
    fromNode->edges = newEdge;
    edgeCount++;

    // Add reverse edge to -> from (undirected graph) if missing
    AdjListNode *toNode = findOrCreateAdjListNode(toId);
    EdgeNode *reverseWalk = toNode->edges;
    while (reverseWalk != nullptr)
    {
        if (strcmp(reverseWalk->toNodeId, fromId) == 0)
        {
            // Reverse edge already present; done
            return;
        }
        reverseWalk = reverseWalk->next;
    }

    EdgeNode *reverseEdge = new EdgeNode();
    strcpy(reverseEdge->toNodeId, fromId);
    reverseEdge->weight = weight;
    strcpy(reverseEdge->connectionType, connType);
    reverseEdge->next = toNode->edges;
    toNode->edges = reverseEdge;
    edgeCount++;
}

// Get edges for a node
EdgeNode *City::getEdges(const char *nodeId) const
{
    AdjListNode *current = adjacencyListHead;

    while (current != nullptr)
    {
        if (strcmp(current->nodeId, nodeId) == 0)
        {
            return current->edges;
        }
        current = current->next;
    }
    return nullptr;
}

// Load locations from CSV file
bool City::loadLocations(const char *filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "Error: Could not open locations file: " << filePath << std::endl;
        return false;
    }

    char line[2048];
    bool firstLine = true;
    int lineCount = 0;

    while (file.getline(line, 2048))
    {
        lineCount++;

        if (firstLine)
        {
            firstLine = false;
            continue;
        }

        if (line[0] == '\0')
            continue;

        char fields[30][MAX_STRING_LENGTH];
        int fieldCount;

        if (!parseCsvLine(line, fields, fieldCount) || fieldCount < 19)
        {
            continue;
        }

        // Dynamically allocate new node
        Node *node = new Node();

        removeQuotes(fields[0]);
        strcpy(node->zone, fields[0]);

        removeQuotes(fields[1]);
        strcpy(node->colony, fields[1]);

        removeQuotes(fields[2]);
        node->streetNo = atoi(fields[2]);

        removeQuotes(fields[3]);
        strcpy(node->street, fields[3]);

        removeQuotes(fields[4]);
        strcpy(node->locationName, fields[4]);

        removeQuotes(fields[5]);
        strcpy(node->locationType, fields[5]);

        removeQuotes(fields[6]);
        node->nodeNo = atoi(fields[6]);

        removeQuotes(fields[7]);
        strcpy(node->id, fields[7]);

        removeQuotes(fields[8]);
        node->x = atof(fields[8]);

        removeQuotes(fields[9]);
        node->y = atof(fields[9]);

        // List grows dynamically
        insertNode(node);

        // Create undirected edge to connected street node
        removeQuotes(fields[15]);
        if (fields[15][0] != '\0' && strcmp(fields[15], "-") != 0)
        {
            removeQuotes(fields[18]);
            double weight = atof(fields[18]);
            addEdge(node->id, fields[15], weight, "Location Edge");
        }
    }

    file.close();
    std::cout << "Loaded " << nodeCount << " location nodes (list grew dynamically)" << std::endl;
    return true;
}

// Load paths from CSV file
bool City::loadPaths(const char *filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        std::cerr << "Error: Could not open paths file: " << filePath << std::endl;
        return false;
    }

    char line[2048];
    bool firstLine = true;
    int lineCount = 0;

    while (file.getline(line, 2048))
    {
        lineCount++;

        if (firstLine)
        {
            firstLine = false;
            continue;
        }

        if (line[0] == '\0')
            continue;

        char fields[30][MAX_STRING_LENGTH];
        int fieldCount;

        if (!parseCsvLine(line, fields, fieldCount) || fieldCount < 18)
        {
            continue;
        }

        // Check if "Connected To Zone" is "No Zone" - if so, skip this line
        char connectedToZone[MAX_STRING_LENGTH];
        strcpy(connectedToZone, fields[9]);
        removeQuotes(connectedToZone);
        if (strcmp(connectedToZone, "No Zone") == 0)
        {
            continue;
        }

        removeQuotes(fields[5]);
        char fromNodeId[MAX_STRING_LENGTH];
        strcpy(fromNodeId, fields[5]);

        removeQuotes(fields[14]);
        char toNodeId[MAX_STRING_LENGTH];
        strcpy(toNodeId, fields[14]);

        removeQuotes(fields[8]);
        char connectionType[MAX_STRING_LENGTH];
        strcpy(connectionType, fields[8]);

        if (fromNodeId[0] == '\0' || toNodeId[0] == '\0' ||
            strcmp(fromNodeId, "-") == 0 || strcmp(toNodeId, "-") == 0)
        {
            continue;
        }

        // Create from node if it doesn't exist (list grows dynamically)
        if (getNode(fromNodeId) == nullptr)
        {
            Node *node = new Node();
            strcpy(node->id, fromNodeId);

            removeQuotes(fields[0]);
            strcpy(node->zone, fields[0]);

            removeQuotes(fields[1]);
            strcpy(node->colony, fields[1]);

            removeQuotes(fields[2]);
            node->streetNo = atoi(fields[2]);

            removeQuotes(fields[3]);
            strcpy(node->street, fields[3]);

            removeQuotes(fields[4]);
            node->nodeNo = atoi(fields[4]);

            removeQuotes(fields[6]);
            node->x = atof(fields[6]);

            removeQuotes(fields[7]);
            node->y = atof(fields[7]);

            strcpy(node->locationType, "street");
            insertNode(node);
        }

        // Create to node if it doesn't exist (list grows dynamically)
        if (getNode(toNodeId) == nullptr)
        {
            Node *node = new Node();
            strcpy(node->id, toNodeId);

            removeQuotes(fields[9]);
            strcpy(node->zone, fields[9]);

            removeQuotes(fields[10]);
            strcpy(node->colony, fields[10]);

            removeQuotes(fields[12]);
            node->streetNo = atoi(fields[12]);

            removeQuotes(fields[11]);
            strcpy(node->street, fields[11]);

            removeQuotes(fields[13]);
            node->nodeNo = atoi(fields[13]);

            removeQuotes(fields[15]);
            node->x = atof(fields[15]);

            removeQuotes(fields[16]);
            node->y = atof(fields[16]);

            strcpy(node->locationType, "street");
            insertNode(node);
        }

        removeQuotes(fields[17]);
        double weight = atof(fields[17]);
        addEdge(fromNodeId, toNodeId, weight, connectionType);
    }

    file.close();
    std::cout << "Loaded " << edgeCount << " edges (bidirectional, grown dynamically)" << std::endl;
    return true;
}

// Get node by ID (searches dynamic linked list)
Node *City::getNode(const char *nodeId) const
{
    Node *current = nodeListHead;

    while (current != nullptr)
    {
        if (strcmp(current->id, nodeId) == 0)
        {
            return current;
        }
        current = current->next;
    }
    return nullptr;
}

// Get nodes by location type (searches dynamic linked list)
void City::getNodesByType(const char *locationType, Node *results[], int &count, int maxResults) const
{
    count = 0;
    Node *current = nodeListHead;

    while (current != nullptr && count < maxResults)
    {
        if (strcmp(current->locationType, locationType) == 0)
        {
            results[count++] = current;
        }
        current = current->next;
    }
}

// Get neighbors of a node (returns head of edge list)
EdgeNode *City::getNeighbors(const char *nodeId) const
{
    return getEdges(nodeId);
}

// Get distance between two nodes
double City::getDistance(const char *nodeId1, const char *nodeId2) const
{
    Node *node1 = getNode(nodeId1);
    Node *node2 = getNode(nodeId2);

    if (!node1 || !node2)
    {
        return -1.0;
    }

    return calculateDistance(node1->x, node1->y, node2->x, node2->y);
}

// Find nearest node to coordinates (searches dynamic linked list)
Node *City::findNearestNode(double x, double y) const
{
    Node *nearest = nullptr;
    double minDistance = 1e9;

    Node *current = nodeListHead;
    while (current != nullptr)
    {
        double dist = calculateDistance(x, y, current->x, current->y);
        if (dist < minDistance)
        {
            minDistance = dist;
            nearest = current;
        }
        current = current->next;
    }

    return nearest;
}

// Get node count
int City::getNodeCount() const
{
    return nodeCount;
}

// Get edge count
int City::getEdgeCount() const
{
    return edgeCount;
}

// Print node information
void City::printNodeInfo(const Node *node) const
{
    if (!node)
    {
        std::cout << "Node is null" << std::endl;
        return;
    }

    std::cout << "Node ID: " << node->id << std::endl;
    std::cout << "Zone: " << node->zone << std::endl;
    std::cout << "Colony: " << node->colony << std::endl;
    std::cout << "Street: " << node->street << " (No. " << node->streetNo << ")" << std::endl;
    std::cout << "Location: " << node->locationName << " (" << node->locationType << ")" << std::endl;
    std::cout << "Coordinates: (" << node->x << ", " << node->y << ")" << std::endl;
}

// Get count of unique edges (undirected edges)
int City::getUniqueEdgeCount() const
{
    return edgeCount / 2;
}

// Get first node in list (for iteration)
Node *City::getFirstNode() const
{
    return nodeListHead;
}

// A* shortest path returning PathResult
PathResult City::findShortestPathAStar(const char *startNodeId, const char *endNodeId) const
{
    PathResult result;

    if (!startNodeId || !endNodeId || nodeCount <= 0)
    {
        return result;
    }

    // Build indexable array of nodes
    int n = nodeCount;
    Node **nodes = new Node *[n];
    int idx = 0;
    Node *curNode = nodeListHead;
    while (curNode && idx < n)
    {
        nodes[idx++] = curNode;
        curNode = curNode->next;
    }
    n = idx;
    if (n == 0)
    {
        delete[] nodes;
        return result;
    }

    // Linear search for node index by ID
    auto findIndexById = [&](const char *id) -> int {
        for (int i = 0; i < n; ++i)
        {
            if (std::strcmp(nodes[i]->id, id) == 0)
                return i;
        }
        return -1;
    };

    int startIndex = findIndexById(startNodeId);
    int goalIndex = findIndexById(endNodeId);
    if (startIndex < 0 || goalIndex < 0)
    {
        delete[] nodes;
        return result;
    }

    if (startIndex == goalIndex)
    {
        result.totalDistance = 0.0;
        result.pathLength = 1;
        std::strncpy(result.path[0], nodes[startIndex]->id, MAX_STRING_LENGTH - 1);
        result.path[0][MAX_STRING_LENGTH - 1] = '\0';
        delete[] nodes;
        return result;
    }

    const double INF = 1e18;

    double *gScore = new double[n];
    double *fScore = new double[n];
    int *parent = new int[n];
    int *heap = new int[n];       // binary min-heap of node indices by fScore
    int *heapPos = new int[n];     // position of node in heap, -1 if not in heap
    char *inClosed = new char[n];  // 0/1 flag

    for (int i = 0; i < n; ++i)
    {
        gScore[i] = INF;
        fScore[i] = INF;
        parent[i] = -1;
        heapPos[i] = -1;
        inClosed[i] = 0;
    }

    auto heuristic = [&](int i) -> double {
        double dx = nodes[i]->x - nodes[goalIndex]->x;
        double dy = nodes[i]->y - nodes[goalIndex]->y;
        return std::sqrt(dx * dx + dy * dy);
    };

    // Heap helpers (min-heap on fScore)
    auto heapSwap = [&](int a, int b) {
        int tmp = heap[a];
        heap[a] = heap[b];
        heap[b] = tmp;
        heapPos[heap[a]] = a;
        heapPos[heap[b]] = b;
    };

    auto heapifyUp = [&](int i) {
        while (i > 0)
        {
            int parentIdx = (i - 1) / 2;
            if (fScore[heap[i]] < fScore[heap[parentIdx]])
            {
                heapSwap(i, parentIdx);
                i = parentIdx;
            }
            else
            {
                break;
            }
        }
    };

    auto heapifyDown = [&](int i, int size) {
        while (true)
        {
            int left = 2 * i + 1;
            int right = 2 * i + 2;
            int smallest = i;
            if (left < size && fScore[heap[left]] < fScore[heap[smallest]])
                smallest = left;
            if (right < size && fScore[heap[right]] < fScore[heap[smallest]])
                smallest = right;
            if (smallest != i)
            {
                heapSwap(i, smallest);
                i = smallest;
            }
            else
            {
                break;
            }
        }
    };

    int heapSize = 0;
    gScore[startIndex] = 0.0;
    fScore[startIndex] = heuristic(startIndex);
    heap[heapSize] = startIndex;
    heapPos[startIndex] = heapSize;
    heapSize++;

    bool found = false;

    while (heapSize > 0)
    {
        int current = heap[0];
        // Pop min
        heap[0] = heap[heapSize - 1];
        heapPos[heap[0]] = 0;
        heapPos[current] = -1;
        heapSize--;
        if (heapSize > 0)
            heapifyDown(0, heapSize);

        if (current == goalIndex)
        {
            found = true;
            break;
        }

        inClosed[current] = 1;

        EdgeNode *edge = getNeighbors(nodes[current]->id);
        while (edge)
        {
            int nei = findIndexById(edge->toNodeId);
            if (nei >= 0)
            {
                if (inClosed[nei])
                {
                    edge = edge->next;
                    continue;
                }

                double tentativeG = gScore[current] + edge->weight;
                if (tentativeG < gScore[nei])
                {
                    parent[nei] = current;
                    gScore[nei] = tentativeG;
                    fScore[nei] = tentativeG + heuristic(nei);

                    if (heapPos[nei] == -1)
                    {
                        heap[heapSize] = nei;
                        heapPos[nei] = heapSize;
                        heapSize++;
                        heapifyUp(heapPos[nei]);
                    }
                    else
                    {
                        heapifyUp(heapPos[nei]);
                    }
                }
            }
            edge = edge->next;
        }
    }

    if (!found)
    {
        delete[] nodes;
        delete[] gScore;
        delete[] fScore;
        delete[] parent;
        delete[] heap;
        delete[] heapPos;
        delete[] inClosed;
        return result;
    }

    // Reconstruct path and check capacity (500 entries)
    int length = 0;
    for (int v = goalIndex; v != -1; v = parent[v])
    {
        length++;
        if (v == startIndex)
            break;
    }

    if (length > 500 || (parent[goalIndex] == -1 && goalIndex != startIndex))
    {
        delete[] nodes;
        delete[] gScore;
        delete[] fScore;
        delete[] parent;
        delete[] heap;
        delete[] heapPos;
        delete[] inClosed;
        return result; // capacity issue or no chain
    }

    int *seq = new int[length];
    int pos = length - 1;
    for (int v = goalIndex; v != -1; v = parent[v])
    {
        seq[pos--] = v;
        if (v == startIndex)
            break;
    }

    result.totalDistance = gScore[goalIndex];
    result.pathLength = length;
    for (int i = 0; i < length; ++i)
    {
        std::strncpy(result.path[i], nodes[seq[i]]->id, MAX_STRING_LENGTH - 1);
        result.path[i][MAX_STRING_LENGTH - 1] = '\0';
    }

    delete[] seq;
    delete[] nodes;
    delete[] gScore;
    delete[] fScore;
    delete[] parent;
    delete[] heap;
    delete[] heapPos;
    delete[] inClosed;
    return result;
}