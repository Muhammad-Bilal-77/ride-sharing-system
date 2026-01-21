# Trip Management Component

## 📋 Overview

The Trip component manages the complete lifecycle of a ride-sharing trip from creation to completion, including state management, path tracking, real-time location updates, and payment calculation.

---

## 🎯 Purpose

- Represent individual ride-sharing trips
- Manage trip state machine (6 states)
- Track real-time driver and rider locations
- Calculate payment based on distance and zone
- Enable movement simulation with step-by-step progression

---

## 🔄 State Machine

```
┌──────────┐
│REQUESTED │  Trip created, waiting for driver
└────┬─────┘
     │ assignDriver()
     ▼
┌──────────┐
│ ASSIGNED │  Driver assigned, paths calculated
└────┬─────┘
     │ startPickupMovement()
     ▼
┌─────────────────┐
│PICKUP_IN_       │  Driver moving to pickup location
│PROGRESS         │
└────┬────────────┘
     │ advanceMovement() → pickup reached
     ▼
┌──────────┐
│ ONGOING  │  Rider in vehicle, en route to destination
└────┬─────┘
     │ advanceMovement() → destination reached
     ▼
┌───────────┐
│ COMPLETED │  Trip finished successfully
└───────────┘

     OR
     ▼
┌───────────┐
│ CANCELLED │  Trip terminated before completion
└───────────┘
```

---

## 🏗️ Data Structure

```cpp
class Trip {
private:
    // Basic Information
    int tripId;              // Unique trip identifier
    int riderId;             // Rider who requested trip
    int driverId;            // Assigned driver (-1 if unassigned)
    TripState state;         // Current state in lifecycle
    
    // Location Information
    char pickupNodeId[MAX_STRING_LENGTH];           // Original pickup
    char dropoffNodeId[MAX_STRING_LENGTH];          // Destination
    char effectivePickupNodeId[MAX_STRING_LENGTH];  // Resolved route node
    
    // Real-Time Tracking
    char driverCurrentNodeId[MAX_STRING_LENGTH];    // Driver position
    char riderCurrentNodeId[MAX_STRING_LENGTH];     // Rider position
    
    // Path Information
    PathResult driverToPickupPath;    // Driver → Pickup route
    PathResult pickupToDropoffPath;   // Pickup → Dropoff route
    int currentPathIndex;              // Movement progress (0-based)
    
public:
    // ... methods
};
```

---

## 🔑 Key Methods

### State Transition Methods

#### `bool transitionToAssigned(int driver)`
**Purpose**: Assign driver to trip

**Pre-conditions**: State = REQUESTED

**Actions**:
- Set driverId
- Change state to ASSIGNED
- Record snapshot for rollback

**Returns**: true if successful

#### `bool transitionToPickupInProgress()`
**Purpose**: Begin driver movement to pickup

**Pre-conditions**: 
- State = ASSIGNED
- driverToPickupPath set

**Actions**:
- Change state to PICKUP_IN_PROGRESS
- Initialize currentPathIndex = 0
- Set initial driver location

**Returns**: true if successful

#### `bool transitionToOngoing()`
**Purpose**: Start trip after pickup

**Pre-conditions**: 
- State = PICKUP_IN_PROGRESS
- Driver reached pickup
- pickupToDropoffPath set

**Actions**:
- Change state to ONGOING
- Reset currentPathIndex = 0
- Set rider in vehicle

**Returns**: true if successful

#### `bool transitionToCompleted()`
**Purpose**: Mark trip as finished

**Pre-conditions**: State = ONGOING

**Actions**:
- Change state to COMPLETED
- Record completion time
- Calculate final payment

**Returns**: true if successful

#### `bool transitionToCancelled()`
**Purpose**: Cancel trip

**Pre-conditions**: State != COMPLETED

**Actions**:
- Change state to CANCELLED
- Release driver
- Record cancellation

**Returns**: true if successful

### Movement Methods

#### `bool advanceMovement()`
**Purpose**: Move one step along current path

**Logic**:
```cpp
if (state == PICKUP_IN_PROGRESS) {
    currentPathIndex++;
    if (currentPathIndex >= driverToPickupPath.pathLength) {
        transitionToOngoing();
        return false;  // Pickup complete
    }
    updateDriverLocation(path[currentPathIndex]);
    return true;  // More steps remaining
}
else if (state == ONGOING) {
    currentPathIndex++;
    if (currentPathIndex >= pickupToDropoffPath.pathLength) {
        return false;  // Destination reached
    }
    updateBothLocations(path[currentPathIndex]);
    return true;  // More steps remaining
}
```

**Returns**: true if more movement steps remain

#### `void setDriverCurrentNodeId(const char *nodeId)`
**Purpose**: Update driver's real-time location

**Used during**: Movement simulation

#### `void setRiderCurrentNodeId(const char *nodeId)`
**Purpose**: Update rider's real-time location

**Used during**: ONGOING state movement

---

## 💰 Payment Calculation

### Formula

**Base Fare** = (Total Distance / 1000) × 150 Rupees
- Rate: 150 Rupees per kilometer
- Distance: Sum of driver-to-pickup + pickup-to-dropoff

**Cross-Zone Surcharge** = 100 Rupees (if pickup and dropoff in different zones)

**Total Fare** = Base Fare + Surcharge

### Methods

#### `double calculateBaseFare() const`
```cpp
double totalDistance = getTotalDistance();
return (totalDistance / 1000.0) * 150.0;
```

#### `double calculateZoneSurcharge() const`
```cpp
char pickupZone[256], dropoffZone[256];
extractZone(pickupNodeId, pickupZone);
extractZone(dropoffNodeId, dropoffZone);

if (strcmp(pickupZone, dropoffZone) != 0)
    return 100.0;
return 0.0;
```

#### `double calculateTotalFare() const`
```cpp
return calculateBaseFare() + calculateZoneSurcharge();
```

### Zone Extraction

```cpp
void extractZone(const char *nodeId, char *zone, int maxLen)
{
    // Example: "zone4_township-B7_S6_Loc9" → "zone4"
    
    // Find underscore position
    int len = 0;
    while (nodeId[len] && nodeId[len] != '_' && len < maxLen - 1)
        len++;
    
    // Copy zone portion
    strncpy(zone, nodeId, len);
    zone[len] = '\0';
}
```

---

## 📊 Data Flow

```
┌─────────────────┐
│  Create Trip    │ ← Constructor
│  (REQUESTED)    │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  Assign Driver  │ ← transitionToAssigned()
│  (ASSIGNED)     │    + setDriverToPickupPath()
└────────┬────────┘    + setPickupToDropoffPath()
         │
         ▼
┌─────────────────┐
│ Start Pickup    │ ← transitionToPickupInProgress()
│ Movement        │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  Advance Steps  │ ← advanceMovement() loop
│  (Pickup Phase) │    Update driver location each step
└────────┬────────┘
         │ Pickup reached
         ▼
┌─────────────────┐
│  Begin Trip     │ ← transitionToOngoing()
│  (ONGOING)      │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  Advance Steps  │ ← advanceMovement() loop
│  (Trip Phase)   │    Update both locations each step
└────────┬────────┘
         │ Destination reached
         ▼
┌─────────────────┐
│ Complete Trip   │ ← transitionToCompleted()
│ (COMPLETED)     │    Calculate payment
└─────────────────┘
```

---

## 🔧 Usage Examples

### Example 1: Create and Assign Trip

```cpp
// Create trip
Trip trip(1, 101, "zone4_township-B7_S6_Loc9", "zone3_johar_town-B7_S6_Loc9");

// Assign driver
trip.transitionToAssigned(1);

// Set paths (calculated by DispatchEngine)
PathResult path1 = city.findShortestPathAStar(driverLoc, pickupLoc);
PathResult path2 = city.findShortestPathAStar(pickupLoc, dropoffLoc);

trip.setDriverToPickupPath(path1);
trip.setPickupToDropoffPath(path2);
trip.setEffectivePickupNodeId("zone4_township-B7_S6_N9");
```

### Example 2: Simulate Movement

```cpp
// Start pickup movement
trip.transitionToPickupInProgress();

// Move driver to pickup (with 2-second delays)
while (trip.advanceMovement() && trip.getState() == PICKUP_IN_PROGRESS) {
    std::cout << "Driver at: " << trip.getDriverCurrentNodeId() << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
}

// Pickup phase automatically transitions to ONGOING

// Continue to destination
while (trip.advanceMovement() && trip.getState() == ONGOING) {
    std::cout << "Driver at: " << trip.getDriverCurrentNodeId() << std::endl;
    std::cout << "Rider at: " << trip.getRiderCurrentNodeId() << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
}

// Trip automatically ready for completion
trip.transitionToCompleted();
```

### Example 3: Calculate Payment

```cpp
Trip *trip = system.getTrip(1);

// Get distance breakdown
double driverToPickup = trip->getDriverToPickupPath().totalDistance;  // 0m
double pickupToDropoff = trip->getPickupToDropoffPath().totalDistance;  // 5648m
double totalDistance = trip->getTotalDistance();  // 5648m

// Calculate payment
double baseFare = trip->calculateBaseFare();           // 847.2 Rupees
double surcharge = trip->calculateZoneSurcharge();     // 100 Rupees (cross-zone)
double totalFare = trip->calculateTotalFare();         // 947.2 Rupees

std::cout << "Distance: " << totalDistance << "m" << std::endl;
std::cout << "Base Fare: " << baseFare << " Rupees" << std::endl;
std::cout << "Surcharge: " << surcharge << " Rupees" << std::endl;
std::cout << "Total: " << totalFare << " Rupees" << std::endl;
```

### Example 4: State Validation

```cpp
// Check current state
std::cout << "State: " << trip.stateToString(trip.getState()) << std::endl;

// Validate transition
if (Trip::isValidTransition(trip.getState(), COMPLETED)) {
    trip.transitionToCompleted();
} else {
    std::cerr << "Invalid state transition" << std::endl;
}
```

---

## 📈 State Transition Rules

| From State | To State | Valid? | Condition |
|-----------|----------|--------|-----------|
| REQUESTED | ASSIGNED | ✅ Yes | Driver available |
| ASSIGNED | PICKUP_IN_PROGRESS | ✅ Yes | Paths calculated |
| PICKUP_IN_PROGRESS | ONGOING | ✅ Yes | Driver reached pickup |
| ONGOING | COMPLETED | ✅ Yes | Reached destination |
| Any | CANCELLED | ✅ Yes | Before COMPLETED |
| COMPLETED | Any | ❌ No | Final state |
| CANCELLED | Any | ❌ No | Final state |

---

## 💡 Design Patterns

### State Pattern
- Encapsulates trip lifecycle states
- Controls valid transitions
- Different behavior per state

### Observer Pattern (Implicit)
- DispatchEngine monitors state changes
- RollbackManager records transitions
- Real-time updates trigger notifications

---

## 🔍 Payment Examples

### Example 1: Short Same-Zone Trip
```
Pickup: zone4_township-B7_S6_Loc9
Dropoff: zone4_township-B7_S6_Loc5
Distance: 109m

Base Fare: (109 / 1000) × 150 = 16.35 Rupees
Surcharge: 0 Rupees (same zone)
Total: 16.35 Rupees
```

### Example 2: Long Cross-Zone Trip
```
Pickup: zone4_township-B7_S6_Loc9
Dropoff: zone3_johar_town-B7_S6_Loc9
Distance: 5648m

Base Fare: (5648 / 1000) × 150 = 847.2 Rupees
Surcharge: 100 Rupees (zone4 → zone3)
Total: 947.2 Rupees
```

### Example 3: Multi-Zone Trip
```
Pickup: zone3_johar_town-B7_S6_Loc5
Dropoff: zone4_township-B1_S6_Loc3
Distance: 4215m

Base Fare: (4215 / 1000) × 150 = 632.25 Rupees
Surcharge: 100 Rupees (zone3 → zone4)
Total: 732.25 Rupees
```

---

## ⚙️ Implementation Details

### Movement Tracking
- `currentPathIndex` starts at 0
- Incremented by `advanceMovement()`
- Compared with `pathLength` to detect completion
- Separate indices for pickup and ongoing phases

### Location Updates
- **Pickup Phase**: Only driver location updates
- **Ongoing Phase**: Both driver and rider locations update
- Locations set to current path node at each step

### Automatic Transitions
- Pickup → Ongoing: When driver reaches pickup
- Ongoing ready for completion: When destination reached
- Requires explicit `transitionToCompleted()` call

---

## 📦 Dependencies

- `city.h`: PathResult structure, MAX_STRING_LENGTH
- `<cstring>`: String operations
- `<iostream>`: Display methods

---

## 🐛 Error Handling

- **Invalid State Transition**: Returns false
- **Path Not Set**: advanceMovement() returns false
- **Null Path**: getTotalDistance() returns 0

---

## 📊 Performance

| Operation | Complexity | Notes |
|-----------|------------|-------|
| State Transition | O(1) | Simple flag update |
| Advance Movement | O(1) | Index increment |
| Calculate Fare | O(1) | Simple arithmetic |
| Extract Zone | O(n) | String search, n = string length |
| Get Total Distance | O(1) | Sum of two values |

---

**File**: `core/trip.h`, `core/trip.cpp`  
**Last Updated**: January 21, 2026
