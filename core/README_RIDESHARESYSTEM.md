# RideShare System Component

## 📋 Overview

The RideShareSystem is the top-level facade that provides a simplified, unified interface to the entire ride-sharing platform. It delegates to the DispatchEngine while hiding implementation complexity.

---

## 🎯 Purpose

- **Unified Interface**: Single entry point for all operations
- **Simplified API**: High-level methods with clear semantics
- **Implementation Hiding**: Abstracts DispatchEngine, City, RollbackManager
- **Convenience**: Reduces boilerplate for common workflows
- **Testing Support**: Easy-to-use interface for test files

---

## 🏗️ Architecture (Facade Pattern)

```
┌─────────────────────────────────────────────────┐
│         RideShareSystem (Facade)                │
│                                                 │
│  Public API:                                    │
│  - addDriver()                                  │
│  - addRider()                                   │
│  - requestTrip()                                │
│  - assignTrip()                                 │
│  - completeTrip()                               │
│  - rollback()                                   │
│  - getTrip() / getDriver() / getRider()         │
│                                                 │
└────────────────────┬────────────────────────────┘
                     │ Delegates to
                     ▼
┌─────────────────────────────────────────────────┐
│         DispatchEngine (Business Logic)         │
│                                                 │
│  - Trip management                              │
│  - Driver assignment                            │
│  - Pickup resolution                            │
│  - Pathfinding                                  │
│  - Rollback coordination                        │
│                                                 │
└──┬────────┬────────┬────────────────────────────┘
   │        │        │
   ▼        ▼        ▼
┌──────┐ ┌──────┐ ┌──────────────┐
│ City │ │Trips │ │RollbackMgr   │
│Graph │ │List  │ │              │
└──────┘ └──────┘ └──────────────┘
```

---

## 🔑 Key Methods

### Initialization

#### `RideShareSystem(City *city, int maxRollbackDepth = 100)`

**Purpose**: Initialize system with city graph

**Parameters**:
- `city`: Pointer to City object (graph)
- `maxRollbackDepth`: Maximum rollback history (default 100)

**Example**:
```cpp
City city;
city.loadLocations("city_locations_path_data/city-locations.csv");
city.loadPaths("city_locations_path_data/paths.csv");

RideShareSystem system(&city, 50);  // Max 50 rollback operations
```

---

### Driver Management

#### `void addDriver(int driverId, const char *name, const char *routeNodeId)`

**Purpose**: Add driver to fleet

**Parameters**:
- `driverId`: Unique driver identifier
- `name`: Driver's name
- `routeNodeId`: Initial location (must be route node)

**Validation**: Checks if routeNodeId is valid route node

**Example**:
```cpp
system.addDriver(1, "Muhammad Ali", "zone4_township-B7_S6_N9");
system.addDriver(2, "Sarah Khan", "zone3_johar_town-B10_S2_N15");
system.addDriver(3, "Ahmed Hassan", "zone2_gulberg-B5_S3_N22");

std::cout << "Fleet size: " << system.getTotalDrivers() << std::endl;
// Output: Fleet size: 3
```

#### `Driver *getDriver(int driverId)`

**Purpose**: Retrieve driver by ID

**Returns**: Pointer to Driver object, or nullptr if not found

**Example**:
```cpp
Driver *driver = system.getDriver(1);
if (driver) {
    std::cout << "Driver: " << driver->getName() << std::endl;
    std::cout << "Location: " << driver->getCurrentLocation() << std::endl;
    std::cout << "Available: " << (driver->getIsAvailable() ? "Yes" : "No") << std::endl;
}
```

---

### Rider Management

#### `void addRider(int riderId, const char *name, const char *phoneNumber)`

**Purpose**: Register rider in system

**Parameters**:
- `riderId`: Unique rider identifier
- `name`: Rider's name
- `phoneNumber`: Contact information

**Example**:
```cpp
system.addRider(101, "Fatima Ahmed", "+92-300-1234567");
system.addRider(102, "Zainab Malik", "+92-321-9876543");

std::cout << "Registered riders: " << system.getTotalRiders() << std::endl;
// Output: Registered riders: 2
```

#### `Rider *getRider(int riderId)`

**Purpose**: Retrieve rider by ID

**Returns**: Pointer to Rider object, or nullptr if not found

**Example**:
```cpp
Rider *rider = system.getRider(101);
if (rider) {
    std::cout << "Rider: " << rider->getName() << std::endl;
    std::cout << "Phone: " << rider->getPhoneNumber() << std::endl;
}
```

---

### Trip Management

#### `Trip *requestTrip(int riderId, const char *pickupNodeId, const char *dropoffNodeId)`

**Purpose**: Create new trip request

**Parameters**:
- `riderId`: Rider requesting trip
- `pickupNodeId`: Pickup location (any node type)
- `dropoffNodeId`: Dropoff destination (any node type)

**Returns**: Pointer to created Trip object

**Internal Actions**:
1. Validate rider exists
2. Create trip (state = REQUESTED)
3. Record rollback snapshot
4. Add to trips list

**Example**:
```cpp
// Residential pickup and dropoff
Trip *trip = system.requestTrip(
    101,  // riderId
    "zone4_township-B7_S6_Loc9",      // Home
    "zone3_johar_town-B7_S6_Loc9"     // Destination
);

std::cout << "Trip ID: " << trip->getTripId() << std::endl;
std::cout << "State: " << trip->stateToString(trip->getState()) << std::endl;

// Output:
// Trip ID: 1
// State: REQUESTED
```

#### `bool assignTrip(int tripId, int driverId)`

**Purpose**: Manually assign specific driver to trip

**Parameters**:
- `tripId`: Trip to assign
- `driverId`: Driver to assign

**Returns**: true if successful, false if invalid

**Internal Actions**:
1. Validate trip exists and state = REQUESTED
2. Validate driver exists and isAvailable = true
3. Resolve pickup location (residential → route node)
4. Calculate paths (driver → pickup, pickup → dropoff)
5. Update trip and driver states
6. Record rollback snapshot

**Example**:
```cpp
// Request trip
Trip *trip = system.requestTrip(101, pickup, dropoff);

// Manually assign driver #1
bool success = system.assignTrip(trip->getTripId(), 1);

if (success) {
    std::cout << "Driver assigned successfully" << std::endl;
} else {
    std::cerr << "Assignment failed" << std::endl;
}
```

#### `bool autoAssignTrip(int tripId)`

**Purpose**: Automatically find and assign nearest available driver

**Parameters**:
- `tripId`: Trip to assign

**Returns**: true if driver found and assigned, false otherwise

**Internal Actions**:
1. Get trip object
2. Resolve pickup location
3. Find nearest available driver using A*
4. Assign driver (same as assignTrip)

**Example**:
```cpp
// Request trip
Trip *trip = system.requestTrip(101, pickup, dropoff);

// Auto-assign nearest driver
bool success = system.autoAssignTrip(trip->getTripId());

if (success) {
    Driver *assignedDriver = system.getDriver(trip->getDriverId());
    std::cout << "Assigned: " << assignedDriver->getName() << std::endl;
} else {
    std::cout << "No available drivers" << std::endl;
}
```

#### `bool completeTrip(int tripId)`

**Purpose**: Mark trip as completed

**Parameters**:
- `tripId`: Trip to complete

**Returns**: true if successful, false if invalid state

**Internal Actions**:
1. Validate trip state = ONGOING
2. Transition trip to COMPLETED
3. Release driver (mark available)
4. Record rollback snapshot

**Example**:
```cpp
// After trip reaches destination
bool completed = system.completeTrip(trip->getTripId());

if (completed) {
    // Calculate payment
    double fare = trip->calculateTotalFare();
    std::cout << "Trip completed. Fare: " << fare << " Rupees" << std::endl;
}
```

#### `bool cancelTrip(int tripId)`

**Purpose**: Cancel trip before completion

**Parameters**:
- `tripId`: Trip to cancel

**Returns**: true if successful, false if already completed

**Example**:
```cpp
// User cancels trip
bool cancelled = system.cancelTrip(trip->getTripId());

if (cancelled) {
    std::cout << "Trip cancelled successfully" << std::endl;
    
    // Driver becomes available again (if was assigned)
    if (trip->getDriverId() != -1) {
        Driver *driver = system.getDriver(trip->getDriverId());
        assert(driver->getIsAvailable() == true);
    }
}
```

#### `Trip *getTrip(int tripId)`

**Purpose**: Retrieve trip by ID

**Returns**: Pointer to Trip object, or nullptr if not found

**Example**:
```cpp
Trip *trip = system.getTrip(1);
if (trip) {
    std::cout << "Trip State: " << trip->stateToString(trip->getState()) << std::endl;
    std::cout << "Distance: " << trip->getTotalDistance() << "m" << std::endl;
    std::cout << "Fare: " << trip->calculateTotalFare() << " Rs" << std::endl;
}
```

---

### Rollback Operations

#### `bool rollbackLastOperation()`

**Purpose**: Undo last recorded operation

**Returns**: true if rollback successful, false if no operations

**Example**:
```cpp
// Create and assign trip
Trip *trip = system.requestTrip(101, pickup, dropoff);
system.autoAssignTrip(trip->getTripId());

// Undo assignment
bool success = system.rollbackLastOperation();

if (success) {
    // Trip back to REQUESTED state
    assert(trip->getState() == REQUESTED);
    assert(trip->getDriverId() == -1);
    
    std::cout << "Assignment rolled back" << std::endl;
}
```

---

### Statistics

#### `int getTotalDrivers() const`
**Returns**: Total number of drivers in fleet

#### `int getAvailableDrivers() const`
**Returns**: Number of drivers currently available

#### `int getTotalTrips() const`
**Returns**: Total trips created

#### `int getActiveTrips() const`
**Returns**: Trips currently in progress (ONGOING)

#### `int getCompletedTrips() const`
**Returns**: Successfully completed trips

**Example**:
```cpp
std::cout << "System Statistics:" << std::endl;
std::cout << "Total Drivers: " << system.getTotalDrivers() << std::endl;
std::cout << "Available Drivers: " << system.getAvailableDrivers() << std::endl;
std::cout << "Total Trips: " << system.getTotalTrips() << std::endl;
std::cout << "Active Trips: " << system.getActiveTrips() << std::endl;
std::cout << "Completed Trips: " << system.getCompletedTrips() << std::endl;

// Output:
// System Statistics:
// Total Drivers: 3
// Available Drivers: 2
// Total Trips: 5
// Active Trips: 1
// Completed Trips: 3
```

---

## 📊 Complete Workflow Example

### End-to-End Trip Simulation

```cpp
// 1. Initialize system
City city;
city.loadLocations("city_locations_path_data/city-locations.csv");
city.loadPaths("city_locations_path_data/paths.csv");

RideShareSystem system(&city);

// 2. Add driver
system.addDriver(1, "Muhammad Ali", "zone4_township-B7_S6_N9");

// 3. Add rider
system.addRider(101, "Fatima Ahmed", "+92-300-1234567");

// 4. Request trip
Trip *trip = system.requestTrip(
    101,
    "zone4_township-B7_S6_Loc9",      // Pickup (residential)
    "zone3_johar_town-B7_S6_Loc9"     // Dropoff (residential)
);

std::cout << "Trip created: #" << trip->getTripId() << std::endl;
std::cout << "State: " << trip->stateToString(trip->getState()) << std::endl;

// 5. Auto-assign driver
bool assigned = system.autoAssignTrip(trip->getTripId());

if (assigned) {
    Driver *driver = system.getDriver(trip->getDriverId());
    std::cout << "Driver assigned: " << driver->getName() << std::endl;
    
    // 6. Start pickup movement
    trip->transitionToPickupInProgress();
    
    // 7. Simulate movement to pickup
    int pickupSteps = 0;
    while (trip->advanceMovement() && trip->getState() == PICKUP_IN_PROGRESS) {
        pickupSteps++;
        std::cout << "Pickup step " << pickupSteps << ": " 
                  << trip->getDriverCurrentNodeId() << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    
    std::cout << "Pickup complete! Pickup steps: " << pickupSteps << std::endl;
    
    // 8. Simulate movement to destination
    int tripSteps = 0;
    while (trip->advanceMovement() && trip->getState() == ONGOING) {
        tripSteps++;
        std::cout << "Trip step " << tripSteps << ": " 
                  << trip->getDriverCurrentNodeId() << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    
    std::cout << "Destination reached! Trip steps: " << tripSteps << std::endl;
    
    // 9. Complete trip
    system.completeTrip(trip->getTripId());
    
    // 10. Display payment
    std::cout << "\n=== TRIP REPORT ===" << std::endl;
    std::cout << "Distance: " << trip->getTotalDistance() << "m" << std::endl;
    std::cout << "Base Fare: " << trip->calculateBaseFare() << " Rs" << std::endl;
    std::cout << "Surcharge: " << trip->calculateZoneSurcharge() << " Rs" << std::endl;
    std::cout << "Total Fare: " << trip->calculateTotalFare() << " Rs" << std::endl;
    
    // 11. Verify system state
    assert(trip->getState() == COMPLETED);
    assert(driver->getIsAvailable() == true);
    assert(driver->getCurrentTripId() == -1);
    
    std::cout << "\n✅ All system features working correctly!" << std::endl;
}
```

**Sample Output**:
```
Trip created: #1
State: REQUESTED
Driver assigned: Muhammad Ali
Pickup step 1: zone4_township-B7_S6_N9
Pickup complete! Pickup steps: 1
Trip step 1: zone4_township-B7_S6_N8
Trip step 2: zone4_township-B7_S5_N7
...
Trip step 75: zone3_johar_town-B7_S6_N9
Destination reached! Trip steps: 75

=== TRIP REPORT ===
Distance: 5648m
Base Fare: 847.2 Rs
Surcharge: 100 Rs (cross-zone)
Total Fare: 947.2 Rs

✅ All system features working correctly!
```

---

## 🔧 Usage Patterns

### Pattern 1: Multiple Drivers, Single Rider

```cpp
// Add 3 drivers at different locations
system.addDriver(1, "Ali", "zone4_township-B7_S6_N9");
system.addDriver(2, "Hassan", "zone3_johar_town-B10_S2_N15");
system.addDriver(3, "Sara", "zone2_gulberg-B5_S3_N22");

// Single rider
system.addRider(101, "Fatima", "+92-300-1111111");

// Request trip
Trip *trip = system.requestTrip(101, pickup, dropoff);

// System finds nearest driver automatically
system.autoAssignTrip(trip->getTripId());

// Nearest driver assigned (Ali, distance = 0m)
assert(trip->getDriverId() == 1);
```

### Pattern 2: Multiple Trips, Sequential Processing

```cpp
// Driver and rider
system.addDriver(1, "Ali", "zone4_township-B7_S6_N9");
system.addRider(101, "Fatima", "+92-300-1111111");

// Trip 1
Trip *trip1 = system.requestTrip(101, "zone4_township-B7_S6_Loc9", "zone3_johar_town-B7_S6_Loc9");
system.autoAssignTrip(trip1->getTripId());
// ... simulate movement ...
system.completeTrip(trip1->getTripId());

// Trip 2 (driver now available again)
Trip *trip2 = system.requestTrip(101, "zone3_johar_town-B7_S6_Loc9", "zone2_gulberg-B5_S3_Loc22");
system.autoAssignTrip(trip2->getTripId());
// ... simulate movement ...
system.completeTrip(trip2->getTripId());
```

### Pattern 3: Rollback on Error

```cpp
// Request and assign trip
Trip *trip = system.requestTrip(101, pickup, dropoff);
system.autoAssignTrip(trip->getTripId());

// Simulate movement
trip->transitionToPickupInProgress();

// Error occurs (e.g., driver unavailable)
if (error) {
    // Rollback assignment
    system.rollbackLastOperation();
    
    // Trip back to REQUESTED
    assert(trip->getState() == REQUESTED);
    
    // Try different driver
    system.assignTrip(trip->getTripId(), 2);
}
```

---

## 💡 Design Advantages

### Facade Pattern Benefits

1. **Simplified Interface**: Single class for all operations
2. **Implementation Hiding**: Users don't need to know about DispatchEngine, City, RollbackManager
3. **Reduced Coupling**: Client code depends only on RideShareSystem
4. **Easy Testing**: Simple API for test files
5. **Flexibility**: Can change internal implementation without affecting client code

### Delegation Pattern

```cpp
class RideShareSystem {
private:
    DispatchEngine *dispatchEngine;  // Delegates to business logic
    
public:
    Trip *requestTrip(...) {
        return dispatchEngine->createTrip(...);
    }
    
    bool assignTrip(...) {
        return dispatchEngine->assignDriverToTrip(...);
    }
    
    // ... all methods delegate to DispatchEngine ...
};
```

---

## 📈 Integration Points

### With DispatchEngine
```cpp
// RideShareSystem delegates to DispatchEngine
class RideShareSystem {
private:
    DispatchEngine *dispatchEngine;
    
public:
    RideShareSystem(City *city, int maxRollback) {
        dispatchEngine = new DispatchEngine(city, maxRollback);
    }
    
    Trip *requestTrip(...) {
        return dispatchEngine->createTrip(...);
    }
};
```

### With City
```cpp
// Passes City to DispatchEngine
RideShareSystem system(&city);
// City used for pathfinding internally
```

### With Test Files
```cpp
// Test files use only RideShareSystem
#include "ridesharesystem.h"

int main() {
    City city;
    city.loadLocations(...);
    city.loadPaths(...);
    
    RideShareSystem system(&city);
    
    // Simple, clean API
    system.addDriver(...);
    system.addRider(...);
    Trip *trip = system.requestTrip(...);
    system.autoAssignTrip(...);
    
    return 0;
}
```

---

## ⚙️ Implementation Details

### Memory Management
```cpp
class RideShareSystem {
private:
    DispatchEngine *dispatchEngine;  // Owned
    
public:
    RideShareSystem(City *city, int maxRollback) {
        dispatchEngine = new DispatchEngine(city, maxRollback);
    }
    
    ~RideShareSystem() {
        delete dispatchEngine;  // Clean up
    }
};
```

### Ownership Model
- **RideShareSystem owns**: DispatchEngine
- **DispatchEngine owns**: RollbackManager
- **RideShareSystem does NOT own**: City (passed by pointer)
- **Collections**: Store pointers, not objects

---

## 📦 Dependencies

- `dispatchengine.h`: Business logic delegation
- `city.h`: City graph (passed to DispatchEngine)
- `trip.h`, `driver.h`, `rider.h`: Entity access

---

## 🐛 Error Handling

### Validation Examples

```cpp
// Invalid driver location (not route node)
system.addDriver(1, "Ali", "zone4_township-B7_S6_Loc9");  // ❌ Residential
// System validates and rejects or resolves

// Invalid trip ID
bool success = system.completeTrip(999);  // Non-existent trip
assert(success == false);

// No available drivers
Trip *trip = system.requestTrip(101, pickup, dropoff);
bool assigned = system.autoAssignTrip(trip->getTripId());
// assigned = false if all drivers busy
```

---

## 📊 Performance

| Operation | Complexity | Notes |
|-----------|------------|-------|
| addDriver() | O(1) | Add to list |
| addRider() | O(1) | Add to list |
| requestTrip() | O(1) | Create trip |
| autoAssignTrip() | O(D × (V+E) log V) | Find nearest driver |
| assignTrip() | O((V+E) log V) | Path calculation |
| completeTrip() | O(T) | Find trip in list |
| rollbackLastOperation() | O(1) | Stack pop + restore |

**Optimization Opportunity**: Use hash maps instead of linked lists for O(1) lookups.

---

## 🌟 API Comparison

### Without Facade (Complex)
```cpp
// User must manage all components
City city;
RollbackManager rollbackMgr;
DispatchEngine dispatchEngine(&city, &rollbackMgr);

Trip *trip = dispatchEngine.createTrip(...);
const char *resolved = dispatchEngine.resolvePickupNode(...);
Driver *driver = dispatchEngine.findNearestDriver(resolved);
dispatchEngine.assignDriverToTrip(trip->getTripId(), driver->getDriverId());
```

### With Facade (Simple)
```cpp
// User interacts with single interface
RideShareSystem system(&city);

Trip *trip = system.requestTrip(...);
system.autoAssignTrip(trip->getTripId());
```

---

## 📝 Best Practices

### 1. Always Check Return Values
```cpp
bool success = system.autoAssignTrip(tripId);
if (!success) {
    std::cerr << "No available drivers" << std::endl;
    // Handle error
}
```

### 2. Validate Objects Before Use
```cpp
Trip *trip = system.getTrip(tripId);
if (trip == nullptr) {
    std::cerr << "Trip not found" << std::endl;
    return;
}
// Safe to use trip
```

### 3. Check State Before Operations
```cpp
if (trip->getState() == ONGOING) {
    system.completeTrip(trip->getTripId());
} else {
    std::cerr << "Cannot complete: trip not ongoing" << std::endl;
}
```

---

**File**: `core/ridesharesystem.h`, `core/ridesharesystem.cpp`  
**Last Updated**: January 21, 2026
