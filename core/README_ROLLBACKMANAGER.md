# Rollback Manager Component

## 📋 Overview

The RollbackManager provides operation undo functionality, enabling the system to revert to previous states by recording snapshots of operations and restoring them when needed.

---

## 🎯 Purpose

- **Operation Tracking**: Record all modifying operations
- **State Snapshots**: Save object states before changes
- **Undo Capability**: Restore previous states
- **History Management**: Maintain operation stack
- **Error Recovery**: Rollback failed operations

---

## 🏗️ Architecture

```
┌─────────────────────────────────────────┐
│         RollbackManager                 │
├─────────────────────────────────────────┤
│                                         │
│  ┌───────────────────────────────────┐ │
│  │    Operation Stack (LIFO)         │ │
│  ├───────────────────────────────────┤ │
│  │  [CREATE_TRIP]    ← Top (most recent)
│  │  [ASSIGN_DRIVER]                  │ │
│  │  [COMPLETE_TRIP]  ← Bottom       │ │
│  └───────────────────────────────────┘ │
│                                         │
│  Each operation contains:               │
│  - Operation type                       │
│  - Trip state snapshot                  │
│  - Driver state snapshot                │
│  - Timestamp                            │
│                                         │
└─────────────────────────────────────────┘
```

---

## 🔑 Operation Types

### Enum Definition
```cpp
enum OperationType {
    CREATE_TRIP,      // New trip created
    ASSIGN_DRIVER,    // Driver assigned to trip
    COMPLETE_TRIP,    // Trip marked completed
    CANCEL_TRIP       // Trip cancelled
};
```

### Operation Details

| Type | When Recorded | State Saved | Rollback Effect |
|------|---------------|-------------|-----------------|
| CREATE_TRIP | Trip created | Trip object | Remove trip from system |
| ASSIGN_DRIVER | Driver assigned | Trip + Driver | Unassign driver, restore REQUESTED |
| COMPLETE_TRIP | Trip completed | Trip + Driver | Restore ONGOING state |
| CANCEL_TRIP | Trip cancelled | Trip + Driver | Restore previous state |

---

## 🏗️ Data Structures

### Operation Snapshot

```cpp
struct OperationSnapshot {
    OperationType type;                 // Operation category
    
    // Trip snapshot
    int tripId;
    TripState tripState;
    int assignedDriverId;
    char effectivePickupNodeId[MAX_STRING_LENGTH];
    
    // Driver snapshot (if applicable)
    int driverId;
    bool driverWasAvailable;
    int driverPreviousTripId;
    
    // Metadata
    time_t timestamp;
    
    OperationSnapshot *next;  // Stack link
};
```

### Stack Structure

```cpp
class RollbackManager {
private:
    OperationSnapshot *operationStack;  // LIFO stack top
    int stackSize;                      // Current depth
    int maxStackSize;                   // Capacity limit
    
public:
    RollbackManager(int maxSize = 100);
    ~RollbackManager();
    
    // Record operations
    void recordCreateTrip(Trip *trip);
    void recordAssignDriver(Trip *trip, Driver *driver);
    void recordCompleteTrip(Trip *trip, Driver *driver);
    void recordCancelTrip(Trip *trip, Driver *driver);
    
    // Rollback
    bool rollback(Trip *trip, Driver *driver);
    
    // Query
    int getStackSize() const;
    bool isEmpty() const;
    OperationType peekLastOperationType() const;
};
```

---

## 🔧 Key Methods

### Recording Operations

#### `void recordCreateTrip(Trip *trip)`

**Purpose**: Save snapshot when trip created

**Snapshot**:
```cpp
snapshot.type = CREATE_TRIP;
snapshot.tripId = trip->getTripId();
snapshot.tripState = REQUESTED;
snapshot.assignedDriverId = -1;
snapshot.timestamp = time(nullptr);
```

**Example**:
```cpp
// Create trip
Trip trip(1, 101, pickup, dropoff);

// Record operation
rollbackManager.recordCreateTrip(&trip);

// Can now rollback to remove trip
```

---

#### `void recordAssignDriver(Trip *trip, Driver *driver)`

**Purpose**: Save states before driver assignment

**Snapshot**:
```cpp
// Trip state before assignment
snapshot.type = ASSIGN_DRIVER;
snapshot.tripId = trip->getTripId();
snapshot.tripState = trip->getState();  // REQUESTED
snapshot.assignedDriverId = trip->getDriverId();  // -1

// Driver state before assignment
snapshot.driverId = driver->getDriverId();
snapshot.driverWasAvailable = driver->getIsAvailable();  // true
snapshot.driverPreviousTripId = driver->getCurrentTripId();  // -1

snapshot.timestamp = time(nullptr);
```

**Example**:
```cpp
// Before assignment
assert(trip->getState() == REQUESTED);
assert(driver->getIsAvailable() == true);

// Record snapshot
rollbackManager.recordAssignDriver(&trip, &driver);

// Perform assignment
trip.transitionToAssigned(driver.getDriverId());
driver.assignTrip(trip.getTripId());

// Can now rollback to restore REQUESTED state
```

---

#### `void recordCompleteTrip(Trip *trip, Driver *driver)`

**Purpose**: Save states before trip completion

**Snapshot**:
```cpp
snapshot.type = COMPLETE_TRIP;
snapshot.tripId = trip->getTripId();
snapshot.tripState = trip->getState();  // ONGOING
snapshot.assignedDriverId = trip->getDriverId();

snapshot.driverId = driver->getDriverId();
snapshot.driverWasAvailable = driver->getIsAvailable();  // false
snapshot.driverPreviousTripId = driver->getCurrentTripId();  // tripId

snapshot.timestamp = time(nullptr);
```

**Example**:
```cpp
// Before completion
assert(trip->getState() == ONGOING);
assert(driver->getIsAvailable() == false);

// Record snapshot
rollbackManager.recordCompleteTrip(&trip, &driver);

// Complete trip
trip.transitionToCompleted();
driver.completeCurrentTrip();

// Can now rollback to restore ONGOING state
```

---

### Rollback Operation

#### `bool rollback(Trip *trip, Driver *driver)`

**Purpose**: Restore system to previous state

**Algorithm**:
```cpp
bool rollback(Trip *trip, Driver *driver) {
    if (isEmpty()) return false;
    
    // Pop top operation
    OperationSnapshot *op = operationStack;
    operationStack = op->next;
    stackSize--;
    
    // Restore based on type
    switch (op->type) {
        case CREATE_TRIP:
            // Remove trip from system
            // (Handled by DispatchEngine)
            break;
            
        case ASSIGN_DRIVER:
            // Restore trip to REQUESTED
            trip->setState(op->tripState);
            trip->setDriverId(op->assignedDriverId);  // -1
            
            // Restore driver to AVAILABLE
            driver->setAvailable(op->driverWasAvailable);
            driver->setCurrentTripId(op->driverPreviousTripId);
            break;
            
        case COMPLETE_TRIP:
            // Restore trip to ONGOING
            trip->setState(op->tripState);
            
            // Restore driver to BUSY
            driver->setAvailable(op->driverWasAvailable);
            driver->setCurrentTripId(op->driverPreviousTripId);
            break;
    }
    
    delete op;
    return true;
}
```

**Returns**: true if rollback successful, false if stack empty

**Example**:
```cpp
// Perform operations
Trip *trip = dispatchEngine.createTrip(101, pickup, dropoff);
rollbackManager.recordCreateTrip(trip);

Driver *driver = dispatchEngine.findNearestDriver(pickup);
rollbackManager.recordAssignDriver(trip, driver);
trip->transitionToAssigned(driver->getDriverId());

// Undo assignment
bool success = rollbackManager.rollback(trip, driver);

// Verify rollback
assert(success == true);
assert(trip->getState() == REQUESTED);
assert(trip->getDriverId() == -1);
assert(driver->getIsAvailable() == true);
```

---

## 📊 Data Flow

### Record Operation Flow

```
1. Operation Occurs
   └─ Example: assignDriverToTrip()

2. Capture Pre-State
   ├─ Trip state: REQUESTED
   ├─ Trip driverId: -1
   ├─ Driver available: true
   └─ Driver tripId: -1

3. Create Snapshot
   ├─ type: ASSIGN_DRIVER
   ├─ tripId: 1
   ├─ tripState: REQUESTED
   ├─ assignedDriverId: -1
   ├─ driverId: 1
   ├─ driverWasAvailable: true
   ├─ driverPreviousTripId: -1
   └─ timestamp: 1705827600

4. Push to Stack
   ├─ snapshot.next = operationStack
   ├─ operationStack = snapshot
   └─ stackSize++

5. Perform Operation
   ├─ trip.transitionToAssigned(driverId)
   └─ driver.assignTrip(tripId)

6. Post-State
   ├─ Trip state: ASSIGNED
   ├─ Trip driverId: 1
   ├─ Driver available: false
   └─ Driver tripId: 1
```

### Rollback Flow

```
1. Rollback Request
   └─ rollbackManager.rollback(trip, driver)

2. Pop Snapshot
   ├─ snapshot = operationStack
   ├─ operationStack = snapshot.next
   └─ stackSize--

3. Read Snapshot
   ├─ type: ASSIGN_DRIVER
   ├─ tripState: REQUESTED
   ├─ assignedDriverId: -1
   ├─ driverWasAvailable: true
   └─ driverPreviousTripId: -1

4. Restore Trip
   ├─ trip.setState(REQUESTED)
   └─ trip.setDriverId(-1)

5. Restore Driver
   ├─ driver.setAvailable(true)
   └─ driver.setCurrentTripId(-1)

6. Cleanup
   └─ delete snapshot

7. Verification
   ├─ Trip state: REQUESTED ✅
   ├─ Trip driverId: -1 ✅
   ├─ Driver available: true ✅
   └─ Driver tripId: -1 ✅
```

---

## 🔧 Usage Examples

### Example 1: Rollback Trip Assignment

```cpp
RollbackManager rollbackMgr(100);

// Create trip
Trip trip(1, 101, "zone4_township-B7_S6_Loc9", "zone3_johar_town-B7_S6_Loc9");
rollbackMgr.recordCreateTrip(&trip);

// Create driver
Driver driver(1, "Ali", "zone4_township-B7_S6_N9");

// Record assignment snapshot
rollbackMgr.recordAssignDriver(&trip, &driver);

// Assign driver
trip.transitionToAssigned(driver.getDriverId());
driver.assignTrip(trip.getTripId());

std::cout << "Trip state: " << trip.stateToString(trip.getState()) << std::endl;
// Output: Trip state: ASSIGNED

// Rollback
rollbackMgr.rollback(&trip, &driver);

std::cout << "Trip state: " << trip.stateToString(trip.getState()) << std::endl;
// Output: Trip state: REQUESTED

std::cout << "Driver available: " << driver.getIsAvailable() << std::endl;
// Output: Driver available: 1 (true)
```

### Example 2: Multiple Operations Stack

```cpp
RollbackManager rollbackMgr;

// Operation 1: Create trip
rollbackMgr.recordCreateTrip(&trip);

// Operation 2: Assign driver
rollbackMgr.recordAssignDriver(&trip, &driver);
trip.transitionToAssigned(driver.getDriverId());

// Operation 3: Complete trip
rollbackMgr.recordCompleteTrip(&trip, &driver);
trip.transitionToCompleted();

std::cout << "Stack size: " << rollbackMgr.getStackSize() << std::endl;
// Output: Stack size: 3

// Rollback completion (restore to ONGOING)
rollbackMgr.rollback(&trip, &driver);
assert(trip.getState() == ONGOING);

// Rollback assignment (restore to REQUESTED)
rollbackMgr.rollback(&trip, &driver);
assert(trip.getState() == REQUESTED);

// Rollback creation (remove trip)
rollbackMgr.rollback(&trip, &driver);
// Trip should be removed from system
```

### Example 3: Check Before Rollback

```cpp
if (!rollbackMgr.isEmpty()) {
    OperationType lastOp = rollbackMgr.peekLastOperationType();
    
    std::cout << "Last operation: ";
    switch (lastOp) {
        case CREATE_TRIP:
            std::cout << "CREATE_TRIP";
            break;
        case ASSIGN_DRIVER:
            std::cout << "ASSIGN_DRIVER";
            break;
        case COMPLETE_TRIP:
            std::cout << "COMPLETE_TRIP";
            break;
    }
    std::cout << std::endl;
    
    // Safe to rollback
    rollbackMgr.rollback(&trip, &driver);
} else {
    std::cout << "No operations to rollback" << std::endl;
}
```

### Example 4: Stack Capacity Management

```cpp
// Limited capacity
RollbackManager rollbackMgr(3);  // Max 3 operations

// Record 4 operations
rollbackMgr.recordCreateTrip(&trip1);       // Stack: [CREATE_TRIP]
rollbackMgr.recordAssignDriver(&trip1, &d1); // Stack: [CREATE_TRIP, ASSIGN_DRIVER]
rollbackMgr.recordCreateTrip(&trip2);       // Stack: [CREATE_TRIP, ASSIGN_DRIVER, CREATE_TRIP]
rollbackMgr.recordAssignDriver(&trip2, &d2); // Stack: [ASSIGN_DRIVER, CREATE_TRIP, ASSIGN_DRIVER] (oldest removed)

// Only last 3 operations retained
assert(rollbackMgr.getStackSize() == 3);
```

---

## 💡 Design Patterns

### Memento Pattern
- **Originator**: Trip, Driver objects
- **Memento**: OperationSnapshot
- **Caretaker**: RollbackManager
- **Purpose**: Capture and restore object states

### Command Pattern (Implicit)
- **Command**: OperationSnapshot (stores operation info)
- **Execute**: Modify object states
- **Undo**: Rollback operation
- **Receiver**: Trip and Driver objects

---

## 🔍 State Restoration Logic

### ASSIGN_DRIVER Rollback

**Before Assignment**:
```cpp
Trip: state=REQUESTED, driverId=-1
Driver: available=true, tripId=-1
```

**After Assignment**:
```cpp
Trip: state=ASSIGNED, driverId=1
Driver: available=false, tripId=1
```

**After Rollback**:
```cpp
Trip: state=REQUESTED, driverId=-1  ← Restored
Driver: available=true, tripId=-1   ← Restored
```

### COMPLETE_TRIP Rollback

**Before Completion**:
```cpp
Trip: state=ONGOING, driverId=1
Driver: available=false, tripId=1
```

**After Completion**:
```cpp
Trip: state=COMPLETED, driverId=1
Driver: available=true, tripId=-1
```

**After Rollback**:
```cpp
Trip: state=ONGOING, driverId=1     ← Restored
Driver: available=false, tripId=1   ← Restored
```

---

## ⚙️ Implementation Details

### Stack Implementation

```cpp
// LIFO stack using linked list
struct OperationSnapshot {
    // ... fields ...
    OperationSnapshot *next;
};

// Push operation
void push(OperationSnapshot *snapshot) {
    snapshot->next = operationStack;
    operationStack = snapshot;
    stackSize++;
    
    // Enforce capacity
    if (stackSize > maxStackSize) {
        removeBottom();
    }
}

// Pop operation
OperationSnapshot *pop() {
    if (isEmpty()) return nullptr;
    
    OperationSnapshot *top = operationStack;
    operationStack = top->next;
    stackSize--;
    
    return top;
}
```

### Memory Management

```cpp
// Destructor: Clean up all snapshots
~RollbackManager() {
    while (operationStack != nullptr) {
        OperationSnapshot *temp = operationStack;
        operationStack = operationStack->next;
        delete temp;
    }
}
```

---

## 📈 Operation Tracking

### Stack Visualization

```
Time ↓

T1: recordCreateTrip(trip1)
    Stack: [CREATE_TRIP(trip1)]

T2: recordAssignDriver(trip1, driver1)
    Stack: [CREATE_TRIP(trip1), ASSIGN_DRIVER(trip1)]

T3: recordCompleteTrip(trip1, driver1)
    Stack: [CREATE_TRIP(trip1), ASSIGN_DRIVER(trip1), COMPLETE_TRIP(trip1)]

T4: rollback()
    Stack: [CREATE_TRIP(trip1), ASSIGN_DRIVER(trip1)]
    Restored: trip1.state = ONGOING

T5: rollback()
    Stack: [CREATE_TRIP(trip1)]
    Restored: trip1.state = REQUESTED

T6: rollback()
    Stack: []
    Effect: Remove trip1 from system
```

---

## 🐛 Error Handling

### Empty Stack
```cpp
if (rollbackMgr.isEmpty()) {
    std::cerr << "Cannot rollback: no operations recorded" << std::endl;
    return false;
}
```

### Null Pointers
```cpp
void recordAssignDriver(Trip *trip, Driver *driver) {
    if (trip == nullptr || driver == nullptr) {
        std::cerr << "Cannot record: null object" << std::endl;
        return;
    }
    // ... record snapshot ...
}
```

### Stack Overflow
```cpp
// Circular buffer approach
if (stackSize >= maxStackSize) {
    removeOldestSnapshot();  // Remove bottom element
}
pushSnapshot(newSnapshot);
```

---

## 📊 Performance

| Operation | Complexity | Notes |
|-----------|------------|-------|
| recordCreateTrip() | O(1) | Push to stack |
| recordAssignDriver() | O(1) | Push to stack |
| recordCompleteTrip() | O(1) | Push to stack |
| rollback() | O(1) | Pop from stack + state restoration |
| getStackSize() | O(1) | Field access |
| isEmpty() | O(1) | Check if stack == nullptr |
| peekLastOperationType() | O(1) | Access top element |

**Memory Usage**: O(M × N)
- M = max stack size
- N = snapshot size (~500 bytes)

---

## 🔄 Integration Points

### With DispatchEngine
```cpp
// DispatchEngine uses RollbackManager
class DispatchEngine {
private:
    RollbackManager *rollbackMgr;
    
public:
    Trip *createTrip(...) {
        Trip *trip = new Trip(...);
        rollbackMgr->recordCreateTrip(trip);
        return trip;
    }
    
    bool assignDriverToTrip(...) {
        rollbackMgr->recordAssignDriver(trip, driver);
        // ... perform assignment ...
    }
    
    bool rollbackLastOperation() {
        return rollbackMgr->rollback(trip, driver);
    }
};
```

### With Trip
```cpp
// No direct dependency
// RollbackManager reads Trip state via getters
// Restores state via setters
```

### With Driver
```cpp
// No direct dependency
// RollbackManager reads Driver state via getters
// Restores state via setters
```

---

## 📦 Dependencies

- `trip.h`: Trip class and TripState enum
- `driver.h`: Driver class
- `<ctime>`: Timestamp generation

---

## 🌟 Use Cases

### 1. User Cancels Trip Request
```cpp
// User cancels before driver arrives
rollbackMgr.recordAssignDriver(trip, driver);
trip->transitionToAssigned(driver->getDriverId());

// User changes mind
rollbackMgr.rollback(trip, driver);
// Trip back to REQUESTED, driver available
```

### 2. Error in Payment Processing
```cpp
// Trip completed, payment fails
rollbackMgr.recordCompleteTrip(trip, driver);
trip->transitionToCompleted();

// Payment error
if (paymentFailed) {
    rollbackMgr.rollback(trip, driver);
    // Trip back to ONGOING, driver still busy
    // Retry payment
}
```

### 3. Testing and Debugging
```cpp
// Test multiple scenarios without recreating objects
rollbackMgr.recordCreateTrip(trip);

// Scenario A
trip->transitionToAssigned(driver1->getDriverId());
// ... test ...
rollbackMgr.rollback(trip, driver1);

// Scenario B
trip->transitionToAssigned(driver2->getDriverId());
// ... test ...
rollbackMgr.rollback(trip, driver2);
```

---

**File**: `core/rollbackmanager.h`, `core/rollbackmanager.cpp`  
**Last Updated**: January 21, 2026
