# Dispatch Engine Component

## 📋 Overview

The DispatchEngine is the core business logic layer that orchestrates the entire ride-sharing system. It manages trip lifecycle, driver assignment, pickup resolution, pathfinding, and rollback operations.

---

## 🎯 Purpose

- **Trip Management**: Create, assign, cancel, and complete trips
- **Driver Assignment**: Find nearest available driver using A* distance
- **Pickup Resolution**: Resolve residential locations to nearest route nodes
- **Path Calculation**: Compute optimal routes using A* algorithm
- **State Management**: Coordinate state changes across components
- **Rollback Support**: Enable operation undo via RollbackManager

---

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────┐
│           DispatchEngine (Orchestrator)         │
├─────────────────────────────────────────────────┤
│                                                 │
│  ┌──────────┐  ┌─────────┐  ┌─────────────┐   │
│  │  Trips   │  │ Drivers │  │   Riders    │   │
│  │  List    │  │  List   │  │    List     │   │
│  └──────────┘  └─────────┘  └─────────────┘   │
│                                                 │
│  ┌──────────────────────────────────────────┐  │
│  │       City Graph (Pathfinding)           │  │
│  └──────────────────────────────────────────┘  │
│                                                 │
│  ┌──────────────────────────────────────────┐  │
│  │    RollbackManager (Operation Undo)      │  │
│  └──────────────────────────────────────────┘  │
│                                                 │
└─────────────────────────────────────────────────┘
```

---

## 🔑 Core Responsibilities

### 1. Trip Lifecycle Management

```cpp
// Create new trip
Trip *createTrip(int riderId, const char *pickup, const char *dropoff);

// Assign driver to trip
bool assignDriverToTrip(int tripId, int driverId);

// Complete trip
bool completeTrip(int tripId);

// Cancel trip
bool cancelTrip(int tripId);
```

### 2. Driver Management

```cpp
// Find nearest available driver
Driver *findNearestDriver(const char *pickupNode);

// Add driver to system
void addDriver(Driver *driver);

// Get driver by ID
Driver *getDriver(int driverId);
```

### 3. Pickup Resolution

```cpp
// Resolve residential → route node
const char *resolvePickupNode(const char *requestedPickup);

// Implementation:
// - If already route node → return as-is
// - If residential (Loc) → find nearest N node
```

### 4. Pathfinding

```cpp
// Calculate optimal route
PathResult calculatePath(const char *start, const char *end);

// Uses A* algorithm from City component
// Returns: path nodes, total distance, success status
```

### 5. Rollback Operations

```cpp
// Undo last operation
bool rollbackLastOperation();

// Supported operations:
// - CREATE_TRIP
// - ASSIGN_DRIVER
// - COMPLETE_TRIP
```

---

## 📊 Data Flow

### Complete Trip Workflow

```
1. Request Trip
   ├─ Rider: riderId=101
   ├─ Pickup: "zone4_township-B7_S6_Loc9" (residential)
   └─ Dropoff: "zone3_johar_town-B7_S6_Loc9" (residential)
   
2. Create Trip
   ├─ Generate tripId=1
   ├─ State: REQUESTED
   └─ Store in trips list
   
3. Resolve Pickup
   ├─ Input: "zone4_township-B7_S6_Loc9"
   ├─ Query: city.findNearestRouteNode()
   └─ Output: "zone4_township-B7_S6_N9"
   
4. Find Driver
   ├─ Query: findNearestDriver("zone4_township-B7_S6_N9")
   ├─ Calculate: A* distances to all available drivers
   ├─ Select: Minimum distance driver
   └─ Return: Driver ID=1
   
5. Calculate Paths
   ├─ Path 1: Driver location → Pickup node
   │   └─ city.findShortestPathAStar(driverLoc, pickupNode)
   │
   └─ Path 2: Pickup node → Dropoff node
       └─ city.findShortestPathAStar(pickupNode, dropoffNode)
   
6. Assign Driver
   ├─ trip.transitionToAssigned(driverId)
   ├─ trip.setDriverToPickupPath(path1)
   ├─ trip.setPickupToDropoffPath(path2)
   ├─ trip.setEffectivePickupNodeId(resolvedPickup)
   ├─ driver.assignTrip(tripId)
   └─ Record rollback snapshot
   
7. Start Movement
   ├─ trip.transitionToPickupInProgress()
   └─ Begin movement simulation
   
8. Pickup Phase
   ├─ Advance movement: driver → pickup
   ├─ Update driver location each step
   └─ Automatic transition to ONGOING
   
9. Trip Phase
   ├─ Advance movement: pickup → dropoff
   ├─ Update driver + rider locations
   └─ Reach destination
   
10. Complete Trip
    ├─ trip.transitionToCompleted()
    ├─ driver.completeCurrentTrip()
    ├─ Calculate payment
    └─ Record completion snapshot
```

---

## 🔧 Key Methods Detail

### Trip Creation

#### `Trip *createTrip(int riderId, const char *pickup, const char *dropoff)`

**Purpose**: Create new trip request

**Steps**:
1. Generate unique tripId
2. Create Trip object (state = REQUESTED)
3. Add to trips list
4. Record rollback snapshot
5. Return trip pointer

**Example**:
```cpp
Trip *trip = dispatchEngine.createTrip(
    101,  // riderId
    "zone4_township-B7_S6_Loc9",   // pickup (residential)
    "zone3_johar_town-B7_S6_Loc9"  // dropoff (residential)
);

std::cout << "Trip ID: " << trip->getTripId() << std::endl;
std::cout << "State: " << trip->stateToString(trip->getState()) << std::endl;

// Output:
// Trip ID: 1
// State: REQUESTED
```

---

### Driver Assignment

#### `bool assignDriverToTrip(int tripId, int driverId)`

**Purpose**: Assign driver and calculate routes

**Pre-conditions**:
- Trip exists and state = REQUESTED
- Driver exists and isAvailable = true

**Steps**:
1. Get trip and driver objects
2. Resolve pickup location (residential → route node)
3. Calculate path: driver → pickup
4. Calculate path: pickup → dropoff
5. Update trip:
   - Set driverId
   - Set effectivePickupNodeId
   - Set both paths
   - Transition to ASSIGNED
6. Update driver:
   - Mark busy
   - Set currentTripId
7. Record rollback snapshot

**Returns**: true if successful, false if invalid

**Example**:
```cpp
// Create trip
Trip *trip = dispatchEngine.createTrip(101, pickup, dropoff);

// Find nearest driver
Driver *driver = dispatchEngine.findNearestDriver(
    dispatchEngine.resolvePickupNode(pickup)
);

// Assign
bool success = dispatchEngine.assignDriverToTrip(
    trip->getTripId(),
    driver->getDriverId()
);

if (success) {
    std::cout << "Driver " << driver->getName() 
              << " assigned to trip " << trip->getTripId() << std::endl;
}
```

---

### Pickup Resolution

#### `const char *resolvePickupNode(const char *requestedPickup)`

**Purpose**: Convert residential location to nearest route node

**Logic**:
```cpp
const char *resolvePickupNode(const char *requestedPickup) {
    // Check if already route node (contains "_N")
    if (strstr(requestedPickup, "_N") != nullptr) {
        return requestedPickup;  // No resolution needed
    }
    
    // Residential location (contains "_Loc")
    // Find nearest route node with same zone/block/street
    return city->findNearestRouteNode(requestedPickup);
}
```

**Examples**:
```cpp
// Residential → Route Node
const char *resolved1 = resolvePickupNode("zone4_township-B7_S6_Loc9");
// Returns: "zone4_township-B7_S6_N9"

// Already Route Node → No Change
const char *resolved2 = resolvePickupNode("zone3_johar_town-B10_S2_N15");
// Returns: "zone3_johar_town-B10_S2_N15"
```

---

### Nearest Driver Search

#### `Driver *findNearestDriver(const char *pickupNode)`

**Purpose**: Find closest available driver using A* distances

**Algorithm**:
```cpp
Driver *findNearestDriver(const char *pickupNode) {
    Driver *nearest = nullptr;
    double minDistance = INFINITY;
    
    // Iterate all drivers
    for (Driver *driver : driversList) {
        // Skip busy drivers
        if (!driver->getIsAvailable()) continue;
        
        // Calculate A* distance
        PathResult path = city->findShortestPathAStar(
            driver->getCurrentLocation(),
            pickupNode
        );
        
        // Update minimum
        if (path.found && path.totalDistance < minDistance) {
            minDistance = path.totalDistance;
            nearest = driver;
        }
    }
    
    return nearest;  // nullptr if no available drivers
}
```

**Complexity**: O(D × (V + E) log V)
- D = number of drivers
- V = vertices, E = edges in graph

**Example**:
```cpp
// Add 3 drivers at different locations
dispatchEngine.addDriver(new Driver(1, "Ali", "zone4_township-B7_S6_N9"));
dispatchEngine.addDriver(new Driver(2, "Hassan", "zone3_johar_town-B10_S2_N15"));
dispatchEngine.addDriver(new Driver(3, "Sara", "zone2_gulberg-B5_S3_N22"));

// Find nearest to pickup
const char *pickup = "zone4_township-B7_S6_N9";
Driver *nearest = dispatchEngine.findNearestDriver(pickup);

std::cout << "Nearest driver: " << nearest->getName() << std::endl;
// Output: Nearest driver: Ali (distance = 0m, already at location)
```

---

### Trip Completion

#### `bool completeTrip(int tripId)`

**Purpose**: Mark trip as completed

**Steps**:
1. Get trip object
2. Validate state = ONGOING
3. Transition trip to COMPLETED
4. Get assigned driver
5. Release driver (mark available)
6. Record rollback snapshot

**Returns**: true if successful

**Example**:
```cpp
// After trip reaches destination
bool completed = dispatchEngine.completeTrip(1);

if (completed) {
    Trip *trip = dispatchEngine.getTrip(1);
    
    // Calculate payment
    std::cout << "Distance: " << trip->getTotalDistance() << "m" << std::endl;
    std::cout << "Base Fare: " << trip->calculateBaseFare() << " Rs" << std::endl;
    std::cout << "Surcharge: " << trip->calculateZoneSurcharge() << " Rs" << std::endl;
    std::cout << "Total: " << trip->calculateTotalFare() << " Rs" << std::endl;
}
```

---

### Rollback Operations

#### `bool rollbackLastOperation()`

**Purpose**: Undo last recorded operation

**Supported Operations**:
1. **CREATE_TRIP**: Remove trip from list
2. **ASSIGN_DRIVER**: Restore pre-assignment states
3. **COMPLETE_TRIP**: Restore to ONGOING state

**Steps**:
1. Get last operation from RollbackManager
2. Restore states based on operation type
3. Pop operation from stack

**Example**:
```cpp
// Create and assign trip
Trip *trip = dispatchEngine.createTrip(101, pickup, dropoff);
Driver *driver = dispatchEngine.findNearestDriver(resolvedPickup);
dispatchEngine.assignDriverToTrip(trip->getTripId(), driver->getDriverId());

// Undo assignment (rollback to REQUESTED state)
dispatchEngine.rollbackLastOperation();

// Verify rollback
assert(trip->getState() == REQUESTED);
assert(trip->getDriverId() == -1);
assert(driver->getIsAvailable() == true);

std::cout << "Rollback successful!" << std::endl;
```

---

## 💡 Design Patterns

### 1. Facade Pattern
- Provides simplified interface to complex subsystem
- Hides City, RollbackManager, collection management
- Single entry point for all operations

### 2. Orchestrator Pattern
- Coordinates multiple components
- Enforces business logic
- Manages workflow sequencing

### 3. Strategy Pattern (Implicit)
- Pathfinding algorithm pluggable (A* currently)
- Driver selection algorithm modifiable
- Payment calculation extensible

---

## 🔍 Business Rules

### Rule 1: Driver Availability
```cpp
if (!driver->getIsAvailable()) {
    // Cannot assign trip
    // Find another driver
}
```

### Rule 2: State Transitions
```cpp
// Trip must be REQUESTED before assignment
if (trip->getState() != REQUESTED) {
    return false;  // Invalid assignment
}
```

### Rule 3: Pickup Resolution
```cpp
// Drivers must pick up from route nodes
// Residential locations must be resolved
if (isResidential(pickup)) {
    pickup = resolveToRouteNode(pickup);
}
```

### Rule 4: Path Validity
```cpp
// Both paths must be valid before assignment
if (!driverToPickup.found || !pickupToDropoff.found) {
    return false;  // Cannot route
}
```

---

## 📈 Statistics Tracking

### Trip Metrics
```cpp
int getTotalTrips() const;           // All trips created
int getActiveTrips() const;          // ONGOING trips
int getCompletedTrips() const;       // COMPLETED trips
int getCancelledTrips() const;       // CANCELLED trips
double getTotalRevenue() const;      // Sum of all fares
```

### Driver Metrics
```cpp
int getTotalDrivers() const;         // Total fleet size
int getAvailableDrivers() const;     // Ready for assignment
int getBusyDrivers() const;          // Currently on trip
```

---

## 🧪 Integration Example

### Complete System Test

```cpp
// 1. Setup
DispatchEngine dispatchEngine(&city, &rollbackManager);

// 2. Add driver
Driver driver(1, "Muhammad Ali", "zone4_township-B7_S6_N9");
dispatchEngine.addDriver(&driver);

// 3. Add rider
Rider rider(101, "Fatima Ahmed", "+92-300-1234567");
dispatchEngine.addRider(&rider);

// 4. Create trip
Trip *trip = dispatchEngine.createTrip(
    101,
    "zone4_township-B7_S6_Loc9",   // Residential
    "zone3_johar_town-B7_S6_Loc9"  // Residential
);

// 5. Resolve pickup
const char *resolvedPickup = dispatchEngine.resolvePickupNode(
    trip->getPickupNodeId()
);
std::cout << "Resolved: " << resolvedPickup << std::endl;
// Output: zone4_township-B7_S6_N9

// 6. Find driver
Driver *nearestDriver = dispatchEngine.findNearestDriver(resolvedPickup);
std::cout << "Nearest: " << nearestDriver->getName() << std::endl;

// 7. Assign
bool assigned = dispatchEngine.assignDriverToTrip(
    trip->getTripId(),
    nearestDriver->getDriverId()
);

// 8. Start movement
trip->transitionToPickupInProgress();

// 9. Simulate pickup phase
while (trip->advanceMovement() && trip->getState() == PICKUP_IN_PROGRESS) {
    std::cout << "Driver at: " << trip->getDriverCurrentNodeId() << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
}

// 10. Simulate trip phase
while (trip->advanceMovement() && trip->getState() == ONGOING) {
    std::cout << "Position: " << trip->getDriverCurrentNodeId() << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
}

// 11. Complete
dispatchEngine.completeTrip(trip->getTripId());

// 12. Display payment
std::cout << "Total Fare: " << trip->calculateTotalFare() << " Rupees" << std::endl;
```

---

## ⚙️ Implementation Details

### Data Structures

```cpp
class DispatchEngine {
private:
    City *city;                      // Graph for pathfinding
    RollbackManager *rollbackMgr;    // Undo operations
    
    // Collections (linked lists)
    struct TripNode {
        Trip *trip;
        TripNode *next;
    } *tripsList;
    
    struct DriverNode {
        Driver *driver;
        DriverNode *next;
    } *driversList;
    
    struct RiderNode {
        Rider *rider;
        RiderNode *next;
    } *ridersList;
    
    int nextTripId;  // Auto-increment
};
```

### Memory Management
- **Ownership**: DispatchEngine does NOT own objects
- **Pointers**: Stores pointers to external objects
- **Lifetime**: Caller manages object deletion
- **Lists**: Manual linked list implementation

---

## 📦 Dependencies

- `city.h`: Pathfinding and graph queries
- `trip.h`: Trip lifecycle management
- `driver.h`: Driver availability
- `rider.h`: Rider information
- `rollbackmanager.h`: Operation undo

---

## 🐛 Error Handling

### Validation Checks
```cpp
// Trip exists
if (trip == nullptr) return false;

// Driver exists and available
if (driver == nullptr || !driver->getIsAvailable()) return false;

// Valid state
if (trip->getState() != REQUESTED) return false;

// Path found
if (!path.found) return false;
```

### Rollback Safety
```cpp
// Operation snapshot before state change
rollbackMgr->recordOperation(CREATE_TRIP, trip);

// If error occurs, rollback
if (error) {
    rollbackMgr->rollbackLastOperation();
}
```

---

## 📊 Performance

| Operation | Complexity | Notes |
|-----------|------------|-------|
| createTrip() | O(1) | List append |
| assignDriverToTrip() | O(D × (V+E) log V) | Find nearest driver |
| findNearestDriver() | O(D × (V+E) log V) | D drivers, A* per driver |
| resolvePickupNode() | O(1) or O(log N) | Hash lookup or search |
| completeTrip() | O(T) | Find trip in list (T trips) |
| rollbackLastOperation() | O(1) | Pop and restore |

**Optimization Opportunity**: Use hash map for O(1) lookups instead of linked lists.

---

## 🔄 State Management

### Trip State Flow (Managed by DispatchEngine)
```
REQUESTED → assignDriverToTrip() → ASSIGNED
ASSIGNED → trip.transitionToPickupInProgress() → PICKUP_IN_PROGRESS
PICKUP_IN_PROGRESS → trip.advanceMovement() → ONGOING
ONGOING → completeTrip() → COMPLETED
Any → cancelTrip() → CANCELLED
```

### Driver State Flow
```
AVAILABLE → assignDriverToTrip() → BUSY
BUSY → completeTrip() → AVAILABLE
```

---

**File**: `core/dispatchengine.h`, `core/dispatchengine.cpp`  
**Last Updated**: January 21, 2026
