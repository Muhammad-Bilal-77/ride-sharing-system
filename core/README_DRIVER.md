# Driver Management Component

## 📋 Overview

The Driver component represents individual drivers in the ride-sharing system, managing their availability status, location tracking, trip assignments, and route node restrictions.

---

## 🎯 Purpose

- Represent individual drivers in the system
- Enforce route node location restriction
- Track driver availability (busy/available)
- Manage trip assignment status
- Enable location updates for real-time tracking

---

## 🏗️ Data Structure

```cpp
class Driver {
private:
    int driverId;                              // Unique identifier
    char name[MAX_STRING_LENGTH];              // Driver name
    char currentLocationNodeId[MAX_STRING_LENGTH];  // Current position
    bool isAvailable;                          // Availability flag
    int currentTripId;                         // -1 if no active trip

public:
    // Constructor
    Driver(int id, const char *driverName, const char *initialLocation);
    
    // Status management
    void setAvailable(bool available);
    void assignTrip(int tripId);
    void completeCurrentTrip();
    
    // Location management
    void updateLocation(const char *nodeId);
    
    // Getters
    int getDriverId() const;
    const char *getName() const;
    const char *getCurrentLocation() const;
    bool getIsAvailable() const;
    int getCurrentTripId() const;
};
```

---

## 🚫 Route Node Restriction

### Policy
Drivers **MUST** be positioned at route nodes (intersections) at all times.

### Rationale
- **Efficient Routing**: A* pathfinding requires valid graph nodes
- **Consistent State**: All drivers at known, routable positions
- **Simplified Logic**: No need to handle intermediate positions

### Validation
```cpp
// ✅ Valid: Route node
Driver driver(1, "John", "zone4_township-B7_S6_N9");

// ❌ Invalid: Residential location (not on route graph)
Driver driver(1, "John", "zone4_township-B7_S6_Loc9");
```

### Route Node Format
```
zone{N}_{area}-B{X}_S{Y}_N{Z}
    │      │     │   │   └─ Node ID
    │      │     │   └───── Street ID  
    │      │     └───────── Block ID
    │      └─────────────── Area name
    └────────────────────── Zone number
```

**Examples**:
- `zone4_township-B7_S6_N9`
- `zone3_johar_town-B10_S2_N15`
- `zone2_gulberg-B5_S3_N22`

---

## 🔑 Key Methods

### Constructor

#### `Driver(int id, const char *driverName, const char *initialLocation)`
**Purpose**: Initialize new driver

**Parameters**:
- `id`: Unique driver ID
- `driverName`: Driver's name (max 255 chars)
- `initialLocation`: Starting node ID (must be route node)

**Initial State**:
- `isAvailable = true`
- `currentTripId = -1`

**Example**:
```cpp
Driver driver(1, "Muhammad Ali", "zone4_township-B7_S6_N9");
```

### Status Management

#### `void setAvailable(bool available)`
**Purpose**: Update driver availability

**Use Cases**:
- `setAvailable(false)`: Trip assigned, driver now busy
- `setAvailable(true)`: Trip completed, driver now free

**Example**:
```cpp
driver.setAvailable(false);  // Mark busy
// ... handle trip ...
driver.setAvailable(true);   // Mark available
```

#### `void assignTrip(int tripId)`
**Purpose**: Assign trip to driver

**Actions**:
- Set `currentTripId = tripId`
- Set `isAvailable = false`

**Example**:
```cpp
driver.assignTrip(42);  // Assign trip #42
std::cout << "Driver busy: " << !driver.getIsAvailable() << std::endl;  // true
```

#### `void completeCurrentTrip()`
**Purpose**: Release driver from current trip

**Actions**:
- Set `currentTripId = -1`
- Set `isAvailable = true`

**Example**:
```cpp
driver.completeCurrentTrip();
std::cout << "Trip ID: " << driver.getCurrentTripId() << std::endl;  // -1
std::cout << "Available: " << driver.getIsAvailable() << std::endl;  // true
```

### Location Management

#### `void updateLocation(const char *nodeId)`
**Purpose**: Update driver's real-time position

**Constraint**: Must be a valid route node

**Used During**: Trip movement simulation

**Example**:
```cpp
// Movement simulation
const char *path[] = {
    "zone4_township-B7_S6_N9",
    "zone4_township-B7_S5_N8",
    "zone4_township-B7_S4_N7"
};

for (int i = 0; i < 3; i++) {
    driver.updateLocation(path[i]);
    std::cout << "Driver at: " << driver.getCurrentLocation() << std::endl;
}
```

---

## 📊 Data Flow

```
┌─────────────────┐
│  Create Driver  │ ← Constructor(id, name, routeNode)
│  (Available)    │   isAvailable = true
└────────┬────────┘   currentTripId = -1
         │
         ▼
┌─────────────────┐
│ Assign Trip     │ ← assignTrip(tripId)
│ (Busy)          │   isAvailable = false
└────────┬────────┘   currentTripId = tripId
         │
         │ During trip...
         │
         ▼
┌─────────────────┐
│ Update Location │ ← updateLocation(nodeId) [repeated]
│ (Moving)        │   Track real-time position
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ Complete Trip   │ ← completeCurrentTrip()
│ (Available)     │   isAvailable = true
└─────────────────┘   currentTripId = -1
```

---

## 🔧 Usage Examples

### Example 1: Add Driver to System

```cpp
// Initialize driver at route node
Driver driver(1, "Muhammad Ali", "zone4_township-B7_S6_N9");

// Display info
std::cout << "Driver ID: " << driver.getDriverId() << std::endl;
std::cout << "Name: " << driver.getName() << std::endl;
std::cout << "Location: " << driver.getCurrentLocation() << std::endl;
std::cout << "Available: " << (driver.getIsAvailable() ? "Yes" : "No") << std::endl;

// Output:
// Driver ID: 1
// Name: Muhammad Ali
// Location: zone4_township-B7_S6_N9
// Available: Yes
```

### Example 2: Trip Assignment Lifecycle

```cpp
Driver driver(1, "Sarah Khan", "zone3_johar_town-B10_S2_N15");

// Initial state
assert(driver.getIsAvailable() == true);
assert(driver.getCurrentTripId() == -1);

// Assign trip
driver.assignTrip(101);
assert(driver.getIsAvailable() == false);
assert(driver.getCurrentTripId() == 101);

// Complete trip
driver.completeCurrentTrip();
assert(driver.getIsAvailable() == true);
assert(driver.getCurrentTripId() == -1);
```

### Example 3: Real-Time Movement Tracking

```cpp
Driver driver(1, "Ahmed Hassan", "zone4_township-B7_S6_N9");

// Simulate movement along path (75 nodes)
PathResult path = city.findShortestPathAStar(
    driver.getCurrentLocation(),
    "zone3_johar_town-B7_S6_N9"
);

for (int i = 0; i < path.pathLength; i++) {
    // Update location
    driver.updateLocation(path.path[i]);
    
    // Display progress
    std::cout << "Step " << (i+1) << "/" << path.pathLength 
              << ": " << driver.getCurrentLocation() << std::endl;
    
    // Simulate 2-second delay
    std::this_thread::sleep_for(std::chrono::seconds(2));
}

std::cout << "Movement complete!" << std::endl;
```

### Example 4: Driver Availability Check

```cpp
Driver *driver = system.getDriver(1);

if (driver->getIsAvailable()) {
    std::cout << "Driver " << driver->getName() << " is available for trips" << std::endl;
    
    // Can assign new trip
    Trip trip(1, 101, pickupNode, dropoffNode);
    driver->assignTrip(trip.getTripId());
} else {
    std::cout << "Driver busy with trip #" << driver->getCurrentTripId() << std::endl;
}
```

---

## 🌍 Location Types

### Route Nodes (✅ Valid for Drivers)
- **Pattern**: `zone{N}_{area}-B{X}_S{Y}_N{Z}`
- **Purpose**: Intersections in road network
- **Characteristics**:
  - Have edges (connections to other nodes)
  - Used in A* pathfinding
  - Drivers positioned here

**Examples**:
```
zone4_township-B7_S6_N9       ← Intersection
zone3_johar_town-B10_S2_N15   ← Intersection
zone2_gulberg-B5_S3_N22       ← Intersection
```

### Residential Locations (❌ Invalid for Drivers)
- **Pattern**: `zone{N}_{area}-B{X}_S{Y}_Loc{Z}`
- **Purpose**: Residential/commercial destinations
- **Characteristics**:
  - No edges (dead-ends)
  - Cannot route through them
  - Only riders can be here

**Examples**:
```
zone4_township-B7_S6_Loc9     ← Residence
zone3_johar_town-B7_S6_Loc9   ← Residence
zone2_gulberg-B5_S3_Loc15     ← Residence
```

---

## 🔍 State Diagram

```
       ┌────────────┐
       │ AVAILABLE  │ ← Initial state
       │ tripId=-1  │   No active trip
       └─────┬──────┘
             │
             │ assignTrip(id)
             │
             ▼
       ┌────────────┐
       │   BUSY     │ ← Active trip
       │ tripId=X   │   Handling passenger
       └─────┬──────┘
             │
             │ completeCurrentTrip()
             │
             ▼
       ┌────────────┐
       │ AVAILABLE  │ ← Back to available
       │ tripId=-1  │   Ready for next trip
       └────────────┘
```

---

## 💡 Design Decisions

### Why Route Node Restriction?

**Problem**: What if driver is at residential location?
- A* algorithm requires valid start node
- No edges from residential locations
- Cannot calculate path

**Solution**: Drivers only at route nodes
- Always routable positions
- Simplified logic
- Consistent state

### Why Single Trip at a Time?

**Current**: One trip per driver
- Simple availability tracking
- Clear state management
- Easier rollback

**Future**: Could support trip queuing
- `std::queue<int> assignedTrips`
- Process trips sequentially
- More complex state machine

---

## 📈 Availability States

| State | isAvailable | currentTripId | Can Assign? |
|-------|-------------|---------------|-------------|
| Idle | true | -1 | ✅ Yes |
| Busy | false | N (>= 0) | ❌ No |

---

## 🧪 Integration Points

### With DispatchEngine
```cpp
// Find nearest available driver
Driver *driver = dispatchEngine.findNearestDriver(pickupNode);
if (driver) {
    driver->assignTrip(trip.getTripId());
}
```

### With Trip
```cpp
Trip trip(1, riderId, pickup, dropoff);
trip.transitionToAssigned(driver.getDriverId());
driver.assignTrip(trip.getTripId());
```

### With RollbackManager
```cpp
// Save state before assignment
rollbackManager.recordDriverState(driver);
driver.assignTrip(tripId);

// Rollback if needed
rollbackManager.rollbackLastOperation();
// Driver state restored automatically
```

---

## ⚙️ Implementation Details

### String Handling
- Uses `strncpy()` for safe copying
- MAX_STRING_LENGTH = 256 bytes
- Null-termination guaranteed

### Memory Layout
```cpp
sizeof(Driver) = 4 + 256 + 256 + 1 + 4 = 521 bytes
                 │   │     │    │   └─ currentTripId (int)
                 │   │     │    └───── isAvailable (bool)
                 │   │     └────────── currentLocationNodeId (char[256])
                 │   └──────────────── name (char[256])
                 └──────────────────── driverId (int)
```

---

## 📦 Dependencies

- `<cstring>`: String operations (strncpy, strlen)
- `city.h`: MAX_STRING_LENGTH constant

---

## 🐛 Error Handling

- **Invalid Location**: No validation (assumes caller provides valid route node)
- **Double Assignment**: No check (assumes DispatchEngine handles logic)
- **Null Name**: No check (assumes valid input)

**Note**: Error handling delegated to higher-level components (DispatchEngine, RideShareSystem)

---

## 📊 Performance

| Operation | Complexity | Notes |
|-----------|------------|-------|
| Constructor | O(n) | String copy (n = name length) |
| assignTrip() | O(1) | Simple field updates |
| completeCurrentTrip() | O(1) | Simple field updates |
| updateLocation() | O(n) | String copy (n = nodeId length) |
| Getters | O(1) | Direct field access |

---

## 🔄 Typical Lifecycle

```cpp
// 1. Create driver
Driver driver(1, "Ali", "zone4_township-B7_S6_N9");

// 2. System adds to fleet
system.addDriver(&driver);

// 3. Trip request arrives
Trip trip(1, 101, "zone4_township-B7_S6_Loc9", "zone3_johar_town-B7_S6_Loc9");

// 4. Assign to nearest available driver
driver.assignTrip(trip.getTripId());

// 5. Move to pickup (75 steps with 2s delays)
for (int i = 0; i < pickupPath.pathLength; i++) {
    driver.updateLocation(pickupPath.path[i]);
    std::this_thread::sleep_for(std::chrono::seconds(2));
}

// 6. Move to destination
for (int i = 0; i < dropoffPath.pathLength; i++) {
    driver.updateLocation(dropoffPath.path[i]);
    std::this_thread::sleep_for(std::chrono::seconds(2));
}

// 7. Complete trip
driver.completeCurrentTrip();

// 8. Driver available for next trip
assert(driver.getIsAvailable() == true);
```

---

**File**: `core/driver.h`, `core/driver.cpp`  
**Last Updated**: January 21, 2026
