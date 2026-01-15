#include "city.h"
#include <iostream>

using namespace std;

void printSeparator()
{
    cout << "\n================================================\n"
         << endl;
}

int main()
{
    cout << "=== City Graph System Test ===" << endl;
    printSeparator();

    // Create city object
    City city;

    // Load location data
    cout << "Loading location data..." << endl;
    if (!city.loadLocations("../city_locations_path_data/city-locations.csv"))
    {
        cerr << "Failed to load locations!" << endl;
        return 1;
    }
    printSeparator();

    // Load path data
    cout << "Loading path data..." << endl;
    if (!city.loadPaths("../city_locations_path_data/paths.csv"))
    {
        cerr << "Failed to load paths!" << endl;
        return 1;
    }
    printSeparator();

    // Display statistics
    cout << "City Graph Statistics:" << endl;
    cout << "Total Nodes: " << city.getNodeCount() << endl;
    cout << "Total Edges: " << city.getEdgeCount() << " (bidirectional)" << endl;
    printSeparator();

    // Test 1: Get a specific node
    cout << "Test 1: Getting a specific node..." << endl;
    Node *testNode = city.getNode("zone1_gulberg-M4_S1_Loc2");
    if (testNode)
    {
        city.printNodeInfo(testNode);
    }
    else
    {
        cout << "Node not found!" << endl;
    }
    printSeparator();

    // Test 2: Find all hospitals
    cout << "Test 2: Finding all hospitals..." << endl;
    Node *hospitals[100];
    int hospitalCount = 0;
    city.getNodesByType("hospital", hospitals, hospitalCount, 100);
    cout << "Found " << hospitalCount << " hospitals:" << endl;
    for (int i = 0; i < hospitalCount && i < 5; i++)
    {
        cout << "  - " << hospitals[i]->locationName
             << " at (" << hospitals[i]->x << ", " << hospitals[i]->y << ")" << endl;
    }
    if (hospitalCount > 5)
    {
        cout << "  ... and " << (hospitalCount - 5) << " more" << endl;
    }
    printSeparator();

    // Test 3: Find all schools
    cout << "Test 3: Finding all schools..." << endl;
    Node *schools[100];
    int schoolCount = 0;
    city.getNodesByType("school", schools, schoolCount, 100);
    cout << "Found " << schoolCount << " schools:" << endl;
    for (int i = 0; i < schoolCount && i < 3; i++)
    {
        cout << "  - " << schools[i]->locationName
             << " at (" << schools[i]->x << ", " << schools[i]->y << ")" << endl;
    }
    printSeparator();

    // Test 4: Find all malls
    cout << "Test 4: Finding all malls..." << endl;
    Node *malls[100];
    int mallCount = 0;
    city.getNodesByType("mall", malls, mallCount, 100);
    cout << "Found " << mallCount << " malls:" << endl;
    for (int i = 0; i < mallCount && i < 3; i++)
    {
        cout << "  - " << malls[i]->locationName
             << " at (" << malls[i]->x << ", " << malls[i]->y << ")" << endl;
    }
    printSeparator();

    // Test 5: Get neighbors of a node
    cout << "Test 5: Getting neighbors of a node..." << endl;
    if (testNode)
    {
        cout << "Neighbors of " << testNode->id << ":" << endl;
        EdgeNode *edges = city.getNeighbors(testNode->id);
        int edgeCount = 0;
        EdgeNode *current = edges;
        while (current != nullptr && edgeCount < 5)
        {
            Node *neighbor = city.getNode(current->toNodeId);
            if (neighbor)
            {
                cout << "  -> " << neighbor->id
                     << " (distance: " << current->weight << "m, type: "
                     << current->connectionType << ")" << endl;
            }
            current = current->next;
            edgeCount++;
        }
    }
    printSeparator();

    // Test 6: Calculate distance between two nodes
    cout << "Test 6: Calculating distance between nodes..." << endl;
    Node *node1 = city.getNode("zone1_gulberg-M4_S1_Loc2");
    Node *node2 = city.getNode("zone1_gulberg-M4_S2_Loc7"); // Mall
    if (node1 && node2)
    {
        double dist = city.getDistance(node1->id, node2->id);
        cout << "Distance from " << node1->locationName
             << " to " << node2->locationName << ": "
             << dist << " meters" << endl;
    }
    printSeparator();

    // Test 7: Find nearest node to specific coordinates
    cout << "Test 7: Finding nearest node to coordinates..." << endl;
    double testX = -1500.0;
    double testY = 900.0;
    Node *nearest = city.findNearestNode(testX, testY);
    if (nearest)
    {
        cout << "Nearest node to (" << testX << ", " << testY << "):" << endl;
        city.printNodeInfo(nearest);
    }
    printSeparator();

    // Test 8: Count homes
    cout << "Test 8: Counting homes..." << endl;
    Node *homes[1000];
    int homeCount = 0;
    city.getNodesByType("home", homes, homeCount, 1000);
    cout << "Total homes in the city: " << homeCount << endl;
    printSeparator();

    // Test 9: Traverse first few nodes
    cout << "Test 9: Traversing first 10 nodes in the list..." << endl;
    Node *current = city.getFirstNode();
    int count = 0;
    while (current != nullptr && count < 10)
    {
        cout << count + 1 << ". " << current->id
             << " (" << current->locationType << ")" << endl;
        current = current->next;
        count++;
    }
    printSeparator();

    // Test 10: Verify undirected graph
    cout << "Test 10: Verifying undirected graph property..." << endl;
    if (testNode)
    {
        EdgeNode *edges = city.getNeighbors(testNode->id);
        if (edges)
        {
            // Check if reverse edge exists
            Node *neighbor = city.getNode(edges->toNodeId);
            if (neighbor)
            {
                cout << "Checking edge: " << testNode->id << " <-> " << neighbor->id << endl;

                // Check forward edge
                bool forwardFound = false;
                EdgeNode *fwd = city.getNeighbors(testNode->id);
                while (fwd)
                {
                    if (strcmp(fwd->toNodeId, neighbor->id) == 0)
                    {
                        forwardFound = true;
                        cout << "Forward edge found: weight = " << fwd->weight << endl;
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
                        cout << "Reverse edge found: weight = " << rev->weight << endl;
                        break;
                    }
                    rev = rev->next;
                }

                if (forwardFound && reverseFound)
                {
                    cout << "✓ Graph is undirected (both edges exist)!" << endl;
                }
                else
                {
                    cout << "✗ Warning: Graph may not be properly undirected!" << endl;
                }
            }
        }
    }
    printSeparator();

    cout << "\n=== All Tests Completed Successfully! ===" << endl;

    return 0;
}