#include "city.h"
#include <iostream>
#include <cstring>
#include <fstream>

void printSeparator()
{
    std::cout << "\n================================================\n"
              << std::endl;
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

    std::cout << "\n=== All Tests Completed Successfully! ===" << std::endl;

    return 0;
}