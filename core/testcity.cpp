#include "city.h"
#include <iostream>
#include <cstring>
#include <fstream>

void printSeparator()
{
    std::cout << "\n================================================\n"
              << std::endl;
}

// Manual Dijkstra (no STL) to cross-check A* output
PathResult manualDijkstraShortestPath(const City &city, const char *startId, const char *goalId)
{
    PathResult res;

    int n = city.getNodeCount();
    if (!startId || !goalId || n <= 0)
        return res;

    Node **nodes = new Node *[n];
    int idx = 0;
    Node *cur = city.getFirstNode();
    while (cur && idx < n)
    {
        nodes[idx++] = cur;
        cur = cur->next;
    }
    n = idx;
    if (n == 0)
    {
        delete[] nodes;
        return res;
    }

    auto findIndexById = [&](const char *id) -> int {
        for (int i = 0; i < n; ++i)
        {
            if (std::strcmp(nodes[i]->id, id) == 0)
                return i;
        }
        return -1;
    };

    int s = findIndexById(startId);
    int g = findIndexById(goalId);
    if (s < 0 || g < 0)
    {
        delete[] nodes;
        return res;
    }

    const double INF = 1e18;
    double *dist = new double[n];
    int *parent = new int[n];
    char *visited = new char[n];
    for (int i = 0; i < n; ++i)
    {
        dist[i] = INF;
        parent[i] = -1;
        visited[i] = 0;
    }

    dist[s] = 0.0;

    for (int iter = 0; iter < n; ++iter)
    {
        int u = -1;
        double best = INF;
        for (int i = 0; i < n; ++i)
        {
            if (!visited[i] && dist[i] < best)
            {
                best = dist[i];
                u = i;
            }
        }
        if (u == -1 || u == g)
            break;

        visited[u] = 1;

        EdgeNode *e = city.getNeighbors(nodes[u]->id);
        while (e)
        {
            int v = findIndexById(e->toNodeId);
            if (v >= 0 && !visited[v])
            {
                double nd = dist[u] + e->weight;
                if (nd < dist[v])
                {
                    dist[v] = nd;
                    parent[v] = u;
                }
            }
            e = e->next;
        }
    }

    if (dist[g] >= INF || (parent[g] == -1 && g != s))
    {
        delete[] nodes;
        delete[] dist;
        delete[] parent;
        delete[] visited;
        return res;
    }

    int len = 0;
    for (int v = g; v != -1; v = parent[v])
    {
        len++;
        if (v == s)
            break;
    }

    if (len > 100)
    {
        delete[] nodes;
        delete[] dist;
        delete[] parent;
        delete[] visited;
        return res;
    }

    int *seq = new int[len];
    int pos = len - 1;
    for (int v = g; v != -1; v = parent[v])
    {
        seq[pos--] = v;
        if (v == s)
            break;
    }

    res.totalDistance = dist[g];
    res.pathLength = len;
    for (int i = 0; i < len; ++i)
    {
        std::strncpy(res.path[i], nodes[seq[i]]->id, MAX_STRING_LENGTH - 1);
        res.path[i][MAX_STRING_LENGTH - 1] = '\0';
    }

    delete[] seq;
    delete[] nodes;
    delete[] dist;
    delete[] parent;
    delete[] visited;
    return res;
}

// Get the paths.csv and city-locations.csv file paths
std::string getDataFilePath(const std::string &filename)
{
    // Try multiple paths to find the data files
    std::string paths[] = {
        std::string("../city_locations_path_data/") + filename,
        std::string("./city_locations_path_data/") + filename,
        std::string("../../city_locations_path_data/") + filename
    };

    for (const auto &path : paths)
    {
        std::ifstream file(path);
        if (file.good())
        {
            file.close();
            return path;
        }
    }

    // Default to the first path if none found (will fail gracefully)
    return paths[0];
}

int main()
{
    std::cout << "=== City Graph System Test ===" << std::endl;
    printSeparator();

    // Create city object
    City city;

    // Load location data
    std::cout << "Loading location data..." << std::endl;
    std::string locationsPath = getDataFilePath("city-locations.csv");
    if (!city.loadLocations(locationsPath.c_str()))
    {
        std::cerr << "Failed to load locations from: " << locationsPath << std::endl;
        return 1;
    }
    printSeparator();

    // Load path data
    std::cout << "Loading path data..." << std::endl;
    std::string pathsPath = getDataFilePath("paths.csv");
    if (!city.loadPaths(pathsPath.c_str()))
    {
        std::cerr << "Failed to load paths from: " << pathsPath << std::endl;
        return 1;
    }
    printSeparator();

    // Display statistics
    std::cout << "City Graph Statistics:" << std::endl;
    std::cout << "Total Nodes: " << city.getNodeCount() << std::endl;
    std::cout << "Unique Edges: " << city.getUniqueEdgeCount() << std::endl;
    std::cout << "Total Directional Edges (bidirectional): " << city.getEdgeCount() << std::endl;
    printSeparator();

    // Test 1: Get a specific node
    std::cout << "Test 1: Getting a specific node..." << std::endl;
    Node *testNode = city.getNode("zone1_gulberg-M4_S1_Loc2");
    if (testNode)
    {
        city.printNodeInfo(testNode);
    }
    else
    {
        std::cout << "Node not found!" << std::endl;
    }
    printSeparator();

    // Test 2: Find all hospitals
    std::cout << "Test 2: Finding all hospitals..." << std::endl;
    const int MAX_LOCATIONS = 200;
    Node *hospitals[MAX_LOCATIONS];
    int hospitalCount = 0;
    city.getNodesByType("hospital", hospitals, hospitalCount, MAX_LOCATIONS);
    std::cout << "Found " << hospitalCount << " hospitals:" << std::endl;
    for (int i = 0; i < hospitalCount && i < 5; i++)
    {
        if (hospitals[i])
        {
            std::cout << "  - " << hospitals[i]->locationName
                      << " at (" << hospitals[i]->x << ", " << hospitals[i]->y << ")" << std::endl;
        }
    }
    if (hospitalCount > 5)
    {
        std::cout << "  ... and " << (hospitalCount - 5) << " more" << std::endl;
    }
    printSeparator();

    // Test 3: Find all schools
    std::cout << "Test 3: Finding all schools..." << std::endl;
    Node *schools[MAX_LOCATIONS];
    int schoolCount = 0;
    city.getNodesByType("school", schools, schoolCount, MAX_LOCATIONS);
    std::cout << "Found " << schoolCount << " schools:" << std::endl;
    for (int i = 0; i < schoolCount && i < 3; i++)
    {
        if (schools[i])
        {
            std::cout << "  - " << schools[i]->locationName
                      << " at (" << schools[i]->x << ", " << schools[i]->y << ")" << std::endl;
        }
    }
    printSeparator();

    // Test 4: Find all malls
    std::cout << "Test 4: Finding all malls..." << std::endl;
    Node *malls[MAX_LOCATIONS];
    int mallCount = 0;
    city.getNodesByType("mall", malls, mallCount, MAX_LOCATIONS);
    std::cout << "Found " << mallCount << " malls:" << std::endl;
    for (int i = 0; i < mallCount && i < 3; i++)
    {
        if (malls[i])
        {
            std::cout << "  - " << malls[i]->locationName
                      << " at (" << malls[i]->x << ", " << malls[i]->y << ")" << std::endl;
        }
    }
    printSeparator();

    // Test 5: Get neighbors of a node
    std::cout << "Test 5: Getting neighbors of a node..." << std::endl;
    if (testNode)
    {
        std::cout << "Neighbors of " << testNode->id << ":" << std::endl;
        EdgeNode *edges = city.getNeighbors(testNode->id);
        int edgeCounter = 0;
        EdgeNode *current = edges;
        while (current != nullptr && edgeCounter < 5)
        {
            Node *neighbor = city.getNode(current->toNodeId);
            if (neighbor)
            {
                std::cout << "  -> " << neighbor->id
                          << " (distance: " << current->weight << "m, type: "
                          << current->connectionType << ")" << std::endl;
            }
            current = current->next;
            edgeCounter++;
        }
    }
    printSeparator();

    // Test 6: Calculate distance between two nodes
    std::cout << "Test 6: Calculating distance between nodes..." << std::endl;
    Node *node1 = city.getNode("zone1_gulberg-M4_S1_Loc2");
    Node *node2 = city.getNode("zone1_gulberg-M4_S2_Loc7"); // Mall
    if (node1 && node2)
    {
        double dist = city.getDistance(node1->id, node2->id);
        std::cout << "Distance from " << node1->locationName
                  << " to " << node2->locationName << ": "
                  << dist << " meters" << std::endl;
    }
    printSeparator();

    // Test 7: Find nearest node to specific coordinates
    std::cout << "Test 7: Finding nearest node to coordinates..." << std::endl;
    double testX = -1500.0;
    double testY = 900.0;
    Node *nearest = city.findNearestNode(testX, testY);
    if (nearest)
    {
        std::cout << "Nearest node to (" << testX << ", " << testY << "):" << std::endl;
        city.printNodeInfo(nearest);
    }
    printSeparator();

    // Test 8: Count homes
    std::cout << "Test 8: Counting homes..." << std::endl;
    const int MAX_HOMES = 4000;
    Node *homes[MAX_HOMES];
    int homeCount = 0;
    city.getNodesByType("home", homes, homeCount, MAX_HOMES);
    std::cout << "Total homes in the city: " << homeCount << std::endl;
    printSeparator();

    // Test 9: Count street nodes
    std::cout << "Test 9: Counting street nodes..." << std::endl;
    int streetCount = 0;
    Node *streetNode = city.getFirstNode();
    while (streetNode != nullptr)
    {
        if (strcmp(streetNode->locationType, "street") == 0)
        {
            streetCount++;
        }
        streetNode = streetNode->next;
    }
    std::cout << "Total street nodes in the city: " << streetCount << std::endl;
    printSeparator();

    // Test 10: Count unique edges manually
    std::cout << "Test 10: Counting unique edges (ensuring no duplicates)..." << std::endl;
    int uniqueEdgeCount = 0;
    Node *edgeCountNode = city.getFirstNode();
    while (edgeCountNode != nullptr)
    {
        EdgeNode *edges = city.getNeighbors(edgeCountNode->id);
        while (edges != nullptr)
        {
            // Only count edge if current node ID is "less than" neighbor ID
            // This prevents counting the same edge twice (A->B and B->A)
            if (strcmp(edgeCountNode->id, edges->toNodeId) < 0)
            {
                uniqueEdgeCount++;
            }
            edges = edges->next;
        }
        edgeCountNode = edgeCountNode->next;
    }
    std::cout << "Unique edges (manual count, no duplicates): " << uniqueEdgeCount << std::endl;
    std::cout << "Total edges reported by City (includes both directions): " << city.getEdgeCount() << std::endl;
    std::cout << "Calculated unique edges (getEdgeCount / 2): " << (city.getEdgeCount() / 2) << std::endl;
    printSeparator();

    // Test 11: Traverse first few nodes
    std::cout << "Test 11: Traversing first 10 nodes in the list..." << std::endl;
    Node *currentNode = city.getFirstNode();
    int nodeCounter = 0;
    while (currentNode != nullptr && nodeCounter < 10)
    {
        std::cout << nodeCounter + 1 << ". " << currentNode->id
                  << " (" << currentNode->locationType << ")" << std::endl;
        currentNode = currentNode->next;
        nodeCounter++;
    }
    printSeparator();

    // Test 12: Count edges for each node (location nodes first, then street nodes)
    std::cout << "Test 12: Edge count per node (showing connections)..." << std::endl;
    std::cout << "\n--- LOCATION NODES ---" << std::endl;
    
    int locationNodesShown = 0;
    Node *nodePtr = city.getFirstNode();
    while (nodePtr != nullptr && locationNodesShown < 10)
    {
        // Show location nodes (not streets)
        if (strcmp(nodePtr->locationType, "street") != 0)
        {
            EdgeNode *edges = city.getNeighbors(nodePtr->id);
            int edgeCountForNode = 0;
            EdgeNode *edgePtr = edges;
            while (edgePtr != nullptr)
            {
                edgeCountForNode++;
                edgePtr = edgePtr->next;
            }
            
            std::cout << "\nNode: " << nodePtr->id << " (" << nodePtr->locationName << ")"
                      << "\n  Type: " << nodePtr->locationType
                      << "\n  Edges: " << edgeCountForNode << std::endl;
            
            // Show all connections for this node
            edgePtr = edges;
            int connIdx = 1;
            while (edgePtr != nullptr)
            {
                Node *neighbor = city.getNode(edgePtr->toNodeId);
                if (neighbor)
                {
                    std::cout << "    " << connIdx << ". -> " << neighbor->id
                              << " (distance: " << edgePtr->weight << "m, type: " << edgePtr->connectionType << ")"
                              << std::endl;
                }
                edgePtr = edgePtr->next;
                connIdx++;
            }
            locationNodesShown++;
        }
        nodePtr = nodePtr->next;
    }
    
    std::cout << "\n--- STREET NODES ---" << std::endl;
    int streetNodesShown = 0;
    nodePtr = city.getFirstNode();
    while (nodePtr != nullptr && streetNodesShown < 10)
    {
        // Show street nodes
        if (strcmp(nodePtr->locationType, "street") == 0)
        {
            EdgeNode *edges = city.getNeighbors(nodePtr->id);
            int edgeCountForNode = 0;
            EdgeNode *edgePtr = edges;
            while (edgePtr != nullptr)
            {
                edgeCountForNode++;
                edgePtr = edgePtr->next;
            }
            
            std::cout << "\nNode: " << nodePtr->id
                      << "\n  Type: street"
                      << "\n  Edges: " << edgeCountForNode << std::endl;
            
            // Show all connections for this node
            edgePtr = edges;
            int connIdx = 1;
            while (edgePtr != nullptr && connIdx <= 3)  // Limit to first 3 connections for streets
            {
                Node *neighbor = city.getNode(edgePtr->toNodeId);
                if (neighbor)
                {
                    std::cout << "    " << connIdx << ". -> " << neighbor->id
                              << " (distance: " << edgePtr->weight << "m)"
                              << std::endl;
                }
                edgePtr = edgePtr->next;
                connIdx++;
            }
            if (edgeCountForNode > 3)
            {
                std::cout << "    ... and " << (edgeCountForNode - 3) << " more connections" << std::endl;
            }
            streetNodesShown++;
        }
        nodePtr = nodePtr->next;
    }
    printSeparator();

    // Test 13: Verify undirected graph
    std::cout << "Test 13: Verifying undirected graph property..." << std::endl;
    if (testNode)
    {
        EdgeNode *edges = city.getNeighbors(testNode->id);
        if (edges)
        {
            // Check if reverse edge exists
            Node *neighbor = city.getNode(edges->toNodeId);
            if (neighbor)
            {
                std::cout << "Checking edge: " << testNode->id << " <-> " << neighbor->id << std::endl;

                // Check forward edge
                bool forwardFound = false;
                EdgeNode *fwd = city.getNeighbors(testNode->id);
                while (fwd)
                {
                    if (strcmp(fwd->toNodeId, neighbor->id) == 0)
                    {
                        forwardFound = true;
                        std::cout << "Forward edge found: weight = " << fwd->weight << std::endl;
                        break;
                    }
                    fwd = fwd->next;
                }

                // Check reverse edge
                bool reverseFound = false;
                EdgeNode *rev = city.getNeighbors(neighbor->id);
                while (rev)
                {
                    if (strcmp(rev->toNodeId, testNode->id) == 0)
                    {
                        reverseFound = true;
                        std::cout << "Reverse edge found: weight = " << rev->weight << std::endl;
                        break;
                    }
                    rev = rev->next;
                }

                if (forwardFound && reverseFound)
                {
                    std::cout << "✓ Graph is undirected (both edges exist)!" << std::endl;
                }
                else
                {
                    std::cout << "✗ Warning: Graph may not be properly undirected!" << std::endl;
                }
            }
        }
    }
    printSeparator();

    // Test 14: Validate A* shortest path against manual Dijkstra (different zones via hospitals)
    // First, collect hospitals by zone
    const char *zone1Hospital = NULL, *zone2Hospital = NULL, *zone3Hospital = NULL, *zone4Hospital = NULL;
    for (int i = 0; i < hospitalCount; ++i)
    {
        if (strcmp(hospitals[i]->zone, "zone1") == 0 && !zone1Hospital)
            zone1Hospital = hospitals[i]->id;
        else if (strcmp(hospitals[i]->zone, "zone2") == 0 && !zone2Hospital)
            zone2Hospital = hospitals[i]->id;
        else if (strcmp(hospitals[i]->zone, "zone3") == 0 && !zone3Hospital)
            zone3Hospital = hospitals[i]->id;
        else if (strcmp(hospitals[i]->zone, "zone4") == 0 && !zone4Hospital)
            zone4Hospital = hospitals[i]->id;
    }
    
    // Try zone transitions: 1->2->3->4
    const char *startId = zone1Hospital ? zone1Hospital : (zone2Hospital ? zone2Hospital : zone4Hospital);
    const char *goalId = zone2Hospital ? zone2Hospital : (zone3Hospital ? zone3Hospital : zone4Hospital);
    
    // Make sure start and goal are different
    if (startId && goalId && strcmp(startId, goalId) == 0)
    {
        if (zone2Hospital && zone3Hospital && strcmp(zone2Hospital, zone3Hospital) != 0)
        {
            startId = zone2Hospital;
            goalId = zone3Hospital;
        }
        else if (zone3Hospital && zone4Hospital && strcmp(zone3Hospital, zone4Hospital) != 0)
        {
            startId = zone3Hospital;
            goalId = zone4Hospital;
        }
    }
    
    std::cout << "Test 14: Shortest path validation (Cross-zone)" << std::endl;
    
    // Find zone for start and goal
    const char *startZone = "", *goalZone = "";
    for (int i = 0; i < hospitalCount; ++i)
    {
        if (strcmp(hospitals[i]->id, startId) == 0) startZone = hospitals[i]->zone;
        if (strcmp(hospitals[i]->id, goalId) == 0) goalZone = hospitals[i]->zone;
    }
    
    std::cout << "Start Hospital: " << startId << " (Zone: " << startZone << ")" << std::endl;
    std::cout << "Goal Hospital: " << goalId << " (Zone: " << goalZone << ")" << std::endl;

    PathResult manualRes = manualDijkstraShortestPath(city, startId, goalId);
    PathResult aStarRes = city.findShortestPathAStar(startId, goalId);

    bool match = true;
    if (manualRes.pathLength != aStarRes.pathLength)
    {
        match = false;
    }
    else
    {
        for (int i = 0; i < manualRes.pathLength; ++i)
        {
            if (std::strcmp(manualRes.path[i], aStarRes.path[i]) != 0)
            {
                match = false;
                break;
            }
        }
    }

    double diff = (manualRes.totalDistance > aStarRes.totalDistance)
                      ? (manualRes.totalDistance - aStarRes.totalDistance)
                      : (aStarRes.totalDistance - manualRes.totalDistance);

    std::cout << "Manual Dijkstra cost: " << manualRes.totalDistance << std::endl;
    std::cout << "A* cost: " << aStarRes.totalDistance << std::endl;
    std::cout << "Path length (manual/A*): " << manualRes.pathLength << "/" << aStarRes.pathLength << std::endl;

    std::cout << "Manual path:" << std::endl;
    for (int i = 0; i < manualRes.pathLength; ++i)
    {
        std::cout << "  " << manualRes.path[i] << std::endl;
    }

    std::cout << "A* path:" << std::endl;
    for (int i = 0; i < aStarRes.pathLength; ++i)
    {
        std::cout << "  " << aStarRes.path[i] << std::endl;
    }

    if (match && diff < 1e-6)
    {
        std::cout << "✓ A* matches manual Dijkstra (cost and path)." << std::endl;
    }
    else if (manualRes.pathLength == 0 || aStarRes.pathLength == 0)
    {
        std::cout << "✗ One of the solvers could not find a path." << std::endl;
    }
    else
    {
        std::cout << "✗ Mismatch detected between A* and manual Dijkstra." << std::endl;
    }
    printSeparator();

    // Test 15: Cross-zone pathfinding from zone 1 to zone 4
    const char *zone1HospId = zone1Hospital;
    const char *zone4HospId = zone4Hospital;
    
    std::cout << "Test 15: Shortest path validation (Zone 1 to Zone 4)" << std::endl;
    
    // Find zone names
    const char *zone1Name = "", *zone4Name = "";
    for (int i = 0; i < hospitalCount; ++i)
    {
        if (strcmp(hospitals[i]->id, zone1HospId) == 0) zone1Name = hospitals[i]->zone;
        if (strcmp(hospitals[i]->id, zone4HospId) == 0) zone4Name = hospitals[i]->zone;
    }
    
    std::cout << "Start Hospital: " << zone1HospId << " (Zone: " << zone1Name << ")" << std::endl;
    std::cout << "Goal Hospital: " << zone4HospId << " (Zone: " << zone4Name << ")" << std::endl;

    PathResult manualRes2 = manualDijkstraShortestPath(city, zone1HospId, zone4HospId);
    PathResult aStarRes2 = city.findShortestPathAStar(zone1HospId, zone4HospId);

    bool match2 = true;
    if (manualRes2.pathLength != aStarRes2.pathLength)
    {
        match2 = false;
    }
    else
    {
        for (int i = 0; i < manualRes2.pathLength; ++i)
        {
            if (std::strcmp(manualRes2.path[i], aStarRes2.path[i]) != 0)
            {
                match2 = false;
                break;
            }
        }
    }

    double diff2 = (manualRes2.totalDistance > aStarRes2.totalDistance)
                       ? (manualRes2.totalDistance - aStarRes2.totalDistance)
                       : (aStarRes2.totalDistance - manualRes2.totalDistance);

    std::cout << "Manual Dijkstra cost: " << manualRes2.totalDistance << std::endl;
    std::cout << "A* cost: " << aStarRes2.totalDistance << std::endl;
    std::cout << "Path length (manual/A*): " << manualRes2.pathLength << "/" << aStarRes2.pathLength << std::endl;

    std::cout << "Manual path:" << std::endl;
    for (int i = 0; i < manualRes2.pathLength; ++i)
    {
        std::cout << "  " << manualRes2.path[i] << std::endl;
    }

    std::cout << "A* path:" << std::endl;
    for (int i = 0; i < aStarRes2.pathLength; ++i)
    {
        std::cout << "  " << aStarRes2.path[i] << std::endl;
    }

    if (match2 && diff2 < 1e-6)
    {
        std::cout << "✓ A* matches manual Dijkstra (cost and path)." << std::endl;
    }
    else if (manualRes2.pathLength == 0 || aStarRes2.pathLength == 0)
    {
        std::cout << "✗ One of the solvers could not find a path." << std::endl;
    }
    else
    {
        std::cout << "✗ Mismatch detected between A* and manual Dijkstra." << std::endl;
    }
    printSeparator();

    std::cout << "\n=== All Tests Completed Successfully! ===" << std::endl;

    return 0;
}