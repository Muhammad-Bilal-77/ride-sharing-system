# City Graph Component

## 📋 Overview

The City component implements a graph-based representation of an urban transportation network using custom data structures without STL. It provides efficient pathfinding, distance calculations, and spatial queries for the ride-sharing system.

---

## 🎯 Purpose

- Model city infrastructure as a directed weighted graph
- Support multiple location types (streets, homes, malls, hospitals)
- Enable efficient shortest-path calculations using A* algorithm
- Organize locations into geographic zones
- Load city data from CSV files dynamically

---

## 🏗️ Data Structures

### Node Structure
```cpp
struct Node {
    char id[MAX_STRING_LENGTH];          // Unique identifier
    double x, y;                          // Coordinates
    char locationType[MAX_STRING_LENGTH]; // "route", "home", "mall", etc.
    char zone[MAX_STRING_LENGTH];         // "zone1", "zone2", etc.
    EdgeNode *edges;                      // Adjacency list
    Node *next;                           // Linked list pointer
};
```

**Node Types**:
- **route**: Streets, highways (driver-accessible)
- **home**: Residential locations
- **mall**: Shopping centers
- **hospital**: Medical facilities
- **park**: Recreational areas

### EdgeNode Structure
```cpp
struct EdgeNode {
    char neighborId[MAX_STRING_LENGTH];  // Destination node
    double weight;                        // Distance in meters
    EdgeNode *next;                       // Next edge in list
};
```

### PathResult Structure
```cpp
struct PathResult {
    Node *path[1000];    // Array of nodes in path
    int pathLength;      // Number of nodes
    double totalDistance;// Total path distance
};
```

---

## 🔄 Logic & Algorithms

### A* Pathfinding Algorithm

**Formula**: `f(n) = g(n) + h(n)`
- `g(n)`: Actual distance from start to node n
- `h(n)`: Heuristic (Euclidean distance to goal)
- `f(n)`: Total estimated cost

**Steps**:
1. Initialize open list with start node
2. While open list not empty:
   - Select node with lowest f-score
   - If goal reached, reconstruct path
   - For each neighbor:
     - Calculate g-score (current + edge weight)
     - Calculate h-score (Euclidean distance)
     - Add to open list if better path found
3. Return path with total distance

**Complexity**: O((V + E) log V) where V = vertices, E = edges

**Heuristic Function**:
```cpp
double h = sqrt((x2 - x1)² + (y2 - y1)²)
```

### Distance Calculation

**Euclidean Distance**:
```cpp
double distance = sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2))
```

Used for:
- A* heuristic
- Direct distance between any two nodes
- Finding nearest nodes

---

## 📊 Data Flow

```
┌─────────────┐
│  CSV Files  │
└──────┬──────┘
       │
       ▼
┌─────────────────┐
│ loadLocations() │──► Parse city-locations.csv
│ loadPaths()     │──► Parse paths.csv
└──────┬──────────┘
       │
       ▼
┌────────────────┐
│  Build Graph   │
│  - Add Nodes   │
│  - Add Edges   │
└──────┬─────────┘
       │
       ▼
┌──────────────────────┐
│   Query Operations   │
│ - getNode()          │
│ - findShortestPath() │
│ - getDistance()      │
│ - findNearestNode()  │
└──────────────────────┘
```

---

## 🔑 Key Methods

### Loading Methods

#### `bool loadLocations(const char *filePath)`
**Purpose**: Load nodes from CSV file

**Process**:
1. Open CSV file
2. Skip header row
3. For each line:
   - Parse: id, x, y, type, zone
   - Create new Node
   - Add to linked list
4. Return success/failure

**Example CSV**:
```csv
id,x,y,type,zone
zone4_township-B7_S6_N9,2304,-1380,route,zone4
zone4_township-B7_S6_Loc9,2304,-1400,home,zone4
```

#### `bool loadPaths(const char *filePath)`
**Purpose**: Load edges from CSV file

**Process**:
1. Open CSV file
2. Skip header row
3. For each line:
   - Parse: from, to, distance
   - Create bidirectional edges
   - Add to adjacency lists
4. Return success/failure

**Example CSV**:
```csv
from,to,distance
zone4_township-B7_S6_N9,zone4_township-B7_S6_N8,22
zone4_township-B7_S6_N8,zone4_township-B7_S6_N7,22
```

### Query Methods

#### `Node* getNode(const char *nodeId)`
**Purpose**: Retrieve node by ID

**Complexity**: O(n)

**Returns**: Pointer to node or nullptr

#### `PathResult findShortestPathAStar(const char *start, const char *end)`
**Purpose**: Calculate optimal route using A*

**Returns**: PathResult with node array and total distance

**Use Cases**:
- Driver to pickup routing
- Pickup to dropoff routing
- Distance estimation

#### `double getDistance(const char *nodeId1, const char *nodeId2)`
**Purpose**: Calculate Euclidean distance

**Returns**: Distance in meters

**Use Cases**:
- Find nearest driver
- Estimate fare
- Calculate proximity

#### `Node* findNearestNode(double x, double y)`
**Purpose**: Find closest node to coordinates

**Complexity**: O(n)

**Returns**: Nearest node pointer

**Use Cases**:
- GPS coordinate matching
- Location approximation

---

## 📈 Statistics

### Current Data (Example City)
- **Total Nodes**: 9076
  - Route nodes: 5044
  - Location nodes: 4032
- **Total Edges**: 20054 (bidirectional)
- **Unique Edges**: 10027
- **Zones**: 4 geographic zones
- **Average Degree**: ~4.4 edges per node

### Memory Usage
- **Node**: ~800 bytes
- **EdgeNode**: ~272 bytes
- **Total Graph**: ~15 MB for 9000+ nodes

---

## 🔧 Usage Examples

### Example 1: Load City
```cpp
City city;

// Load data
if (!city.loadLocations("city_locations_path_data/city-locations.csv")) {
    std::cerr << "Failed to load locations" << std::endl;
    return;
}

if (!city.loadPaths("city_locations_path_data/paths.csv")) {
    std::cerr << "Failed to load paths" << std::endl;
    return;
}

std::cout << "Loaded " << city.getNodeCount() << " nodes" << std::endl;
std::cout << "Loaded " << city.getUniqueEdgeCount() << " edges" << std::endl;
```

### Example 2: Find Shortest Path
```cpp
const char *start = "zone4_township-B7_S6_N9";
const char *end = "zone3_johar_town-B7_S6_Loc9";

PathResult path = city.findShortestPathAStar(start, end);

std::cout << "Path length: " << path.pathLength << " nodes" << std::endl;
std::cout << "Total distance: " << path.totalDistance << " meters" << std::endl;

// Print path
for (int i = 0; i < path.pathLength; i++) {
    std::cout << path.path[i]->id;
    if (i < path.pathLength - 1) std::cout << " → ";
}
```

### Example 3: Calculate Distance
```cpp
double dist = city.getDistance(
    "zone4_township-B7_S6_N9",
    "zone4_township-B7_S6_Loc9"
);

std::cout << "Distance: " << dist << " meters" << std::endl;
```

### Example 4: Find Nearest Node
```cpp
double x = 2304.0;
double y = -1400.0;

Node *nearest = city.findNearestNode(x, y);

if (nearest) {
    std::cout << "Nearest node: " << nearest->id << std::endl;
    std::cout << "Type: " << nearest->locationType << std::endl;
    std::cout << "Zone: " << nearest->zone << std::endl;
}
```

---

## ⚙️ Implementation Details

### CSV Parsing
- Custom tokenization using `strtok()`
- Dynamic memory allocation for nodes and edges
- Bidirectional edge creation (both directions)
- Error handling for malformed data

### Memory Management
- Manual linked list allocation
- Destructor frees all nodes and edges
- No memory leaks (verified)

### Optimization Opportunities
- **Hash Table**: Replace linear node lookup with O(1) hash
- **Spatial Index**: R-tree or quadtree for faster nearest neighbor
- **Edge Compression**: Store only forward edges, infer reverse
- **Memory Pool**: Pre-allocate node/edge blocks

---

## 🔍 Node Type Usage

| Type | Count | Driver Access | Rider Access | Purpose |
|------|-------|---------------|--------------|---------|
| route | 5044 | ✅ Yes | ❌ No | Driver locations, routing |
| home | 2016 | ❌ No | ✅ Yes | Residential pickups/dropoffs |
| mall | 1008 | ❌ No | ✅ Yes | Shopping destinations |
| hospital | 504 | ❌ No | ✅ Yes | Medical facilities |
| park | 504 | ❌ No | ✅ Yes | Recreation areas |

---

## 📦 Dependencies

- `<iostream>`: Console I/O
- `<fstream>`: File operations
- `<cstring>`: String manipulation
- `<cmath>`: Mathematical functions (sqrt, pow)

---

## 🐛 Error Handling

- **File Not Found**: Returns false from load methods
- **Invalid Node ID**: Returns nullptr from getNode()
- **No Path Exists**: Returns PathResult with pathLength = 0
- **Memory Allocation**: Checked with null pointer tests

---

## 📊 Performance Metrics

| Operation | Complexity | Typical Time |
|-----------|------------|--------------|
| Load Locations | O(n) | ~500ms for 9000 nodes |
| Load Paths | O(e) | ~500ms for 10000 edges |
| Node Lookup | O(n) | <1ms |
| A* Pathfinding | O((V+E)log V) | <100ms for 75-node path |
| Distance Calc | O(1) | <1μs |
| Nearest Node | O(n) | ~5ms |

---

**File**: `core/city.h`, `core/city.cpp`  
**Last Updated**: January 21, 2026
