# Ride-Sharing System

A comprehensive C++ ride-sharing system with real-time routing, payment calculation, and rollback capabilities.


## 📋 Table of Contents
- [Overview](#overview)
- [System Architecture](#system-architecture)
- [Key Features](#key-features)
- [Component Documentation](#component-documentation)
- [Frontend/UI Flow & Screens](FRONTEND_UI_FLOW_README.md)
- [Getting Started](#getting-started)
- [Usage Examples](#usage-examples)
- [Technical Specifications](#technical-specifications)

---

## 🎯 Overview

This ride-sharing system implements a complete trip management solution with:
- **Graph-based city modeling** with 9000+ nodes and 10000+ edges
- **Real-time driver tracking** with location updates every 2 seconds
- **A* pathfinding algorithm** for optimal route calculation
- **Payment system** with distance-based fares and cross-zone surcharges
- **Rollback mechanism** for operation reversal and state management
- **State machine** for trip lifecycle management

---

## 🏗️ System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    RideShareSystem                          │
│  (Application Facade - User-facing interface)               │
└──────────────┬──────────────────────────────────────────────┘
               │
               ├─────────────────────────────────────────┐
               │                                         │
    ┌──────────▼──────────┐                  ┌──────────▼──────────┐
    │   DispatchEngine    │                  │   RollbackManager   │
    │  (Business Logic)   │◄─────────────────│  (State History)    │
    └──────────┬──────────┘                  └─────────────────────┘
               │
        ┌──────┼──────┬──────────┐
        │      │      │          │
   ┌────▼─┐ ┌─▼──┐ ┌─▼────┐ ┌──▼──┐
   │ Trip │ │City│ │Driver│ │Rider│
   └──────┘ └────┘ └──────┘ └─────┘
```

### Layer Responsibilities:

1. **RideShareSystem**: High-level API, delegates to DispatchEngine
2. **DispatchEngine**: Core business logic, policy enforcement, rollback integration
3. **RollbackManager**: Operation history and state reversal
4. **Data Models**: City (graph), Trip (lifecycle), Driver, Rider

---

## ✨ Key Features

### 1. City Graph System
- **Nodes**: 9076 locations (routes + destinations)
- **Edges**: 10027 bidirectional connections
- **Node Types**: 
  - Route nodes: streets, highways (driver-accessible)
  - Location nodes: homes, malls, hospitals (rider destinations)
- **Zones**: 4 geographic zones (zone1, zone2, zone3, zone4)

### 2. Trip Management
- **6-State Lifecycle**:
  ```
  REQUESTED → ASSIGNED → PICKUP_IN_PROGRESS → ONGOING → COMPLETED
                                    ↓
                                CANCELLED
  ```
- **Real-time tracking** of driver and rider locations
- **Automatic state transitions** based on movement progress

### 3. Routing & Pathfinding
- **A* Algorithm** with Euclidean heuristic
- **Dual-path calculation**:
  - Driver → Pickup location
  - Pickup → Dropoff location
- **Route optimization** for minimal distance

### 4. Payment System
- **Base Fare**: 150 Rupees per 1000 meters
- **Cross-Zone Surcharge**: +100 Rupees when crossing zones
- **Formula**: `Total = (Distance/1000 × 150) + Surcharge`

### 5. Rollback Mechanism
- **Operation Tracking**: All state changes recorded
- **Snapshot Storage**: Complete state before each operation
- **Selective Rollback**: Undo last K operations
- **Operation Types**: Driver add, trip assign, trip complete, location update

### 6. Real-Time Simulation
- **2-second intervals** between node movements
- **Progressive updates** of driver/rider locations
- **Progress tracking** with percentage completion

---


## 📚 Component Documentation

Detailed documentation for each component:

| Component | File | Description |
|-----------|------|-------------|
| City Graph | [README_CITY.md](core/README_CITY.md) | Graph structure, A* pathfinding, CSV loading |
| Trip Management | [README_TRIP.md](core/README_TRIP.md) | State machine, payment calculation, movement |
| Driver | [README_DRIVER.md](core/README_DRIVER.md) | Driver management, availability, location |
| Rider | [README_RIDER.md](core/README_RIDER.md) | Rider information and trip requests |
| Dispatch Engine | [README_DISPATCHENGINE.md](core/README_DISPATCHENGINE.md) | Business logic, policies, nearest driver |
| Rollback Manager | [README_ROLLBACKMANAGER.md](core/README_ROLLBACKMANAGER.md) | State snapshots, operation reversal |
| RideShare System | [README_RIDESHARESYSTEM.md](core/README_RIDESHARESYSTEM.md) | Main API, system facade |
| Frontend/UI Flow | [FRONTEND_UI_FLOW_README.md](FRONTEND_UI_FLOW_README.md) | In-depth UI/UX, navigation, and screen documentation |

---

## 🚀 Getting Started

### Prerequisites
- C++17 compatible compiler (g++, MSVC, clang)
- MinGW (Windows) or GCC (Linux/Mac)

### Compilation

```bash
# Full system test
g++ -std=c++17 -I. -Icore \
    core/test_complete_system.cpp \
    core/city.cpp \
    core/driver.cpp \
    core/rider.cpp \
    core/trip.cpp \
    core/dispatchengine.cpp \
    core/rollbackmanager.cpp \
    core/ridesharesystem.cpp \
    -o test_complete_system.exe

# Payment test
g++ -std=c++17 -I. -Icore \
    core/test_payment.cpp \
    core/*.cpp \
    -o test_payment.exe

# Movement test with timing
g++ -std=c++17 -I. -Icore \
    core/test_movement.cpp \
    core/*.cpp \
    -o test_movement.exe
```

### Running Tests

```bash
# Complete system demonstration
./test_complete_system.exe

# Payment calculation test
./test_payment.exe

# Full movement simulation (with 2s delays)
./test_movement.exe
```

---

## 💡 Usage Examples

### Example 1: Basic Trip Creation

```cpp
#include "ridesharesystem.h"

int main() {
    // Load city
    City city;
    city.loadLocations("city_locations_path_data/city-locations.csv");
    city.loadPaths("city_locations_path_data/paths.csv");
    
    // Initialize system
    RideShareSystem system(&city);
    
    // Add driver
    system.addDriver(1, "zone4_township-B7_S6_N9", "zone4");
    
    // Create trip
    system.createAndRequestTrip(101, 
        "zone4_township-B7_S6_Loc9",    // pickup
        "zone3_johar_town-B7_S6_Loc9");  // dropoff
    
    // Assign driver
    system.assignTrip(1, 1);
    
    // Get trip details
    Trip *trip = system.getTrip(1);
    double fare = trip->calculateTotalFare();
    
    return 0;
}
```

### Example 2: Real-Time Movement

```cpp
// Start movement
system.startTripMovement(1);

// Simulate movement with 2-second delays
while (system.advanceTrip(1)) {
    Trip *trip = system.getTrip(1);
    
    std::cout << "Driver at: " << trip->getDriverCurrentNodeId() << std::endl;
    std::cout << "State: " << trip->stateToString(trip->getState()) << std::endl;
    
    std::this_thread::sleep_for(std::chrono::seconds(2));
}
```

### Example 3: Payment Calculation

```cpp
Trip *trip = system.getTrip(1);

// Get payment breakdown
double baseFare = trip->calculateBaseFare();           // 847.2 Rupees
double surcharge = trip->calculateZoneSurcharge();     // 100 Rupees (cross-zone)
double totalFare = trip->calculateTotalFare();         // 947.2 Rupees

std::cout << "Base Fare: " << baseFare << " Rupees" << std::endl;
std::cout << "Surcharge: " << surcharge << " Rupees" << std::endl;
std::cout << "Total: " << totalFare << " Rupees" << std::endl;
```

### Example 4: Rollback Operations

```cpp
// Perform operations
system.addDriver(1, "node1", "zone1");
system.addDriver(2, "node2", "zone2");
system.assignTrip(1, 1);

// Rollback last 2 operations
system.rollbackLastKOperations(2);

// Driver #1 is now available again, trip unassigned
```

---

## 🔧 Technical Specifications

### Memory Management
- **No STL**: Pure C++ with manual memory management
- **Dynamic Linked Lists**: Custom implementation for all collections
- **Cleanup**: Proper destructors free all allocated memory

### Data Structures
- **Graph**: Adjacency list representation
- **Priority Queue**: Custom min-heap for A*
- **History Stack**: Linked list of operation snapshots

### Performance
- **A* Pathfinding**: O((V + E) log V) where V = nodes, E = edges
- **Node Lookup**: O(n) linear search (hash table possible enhancement)
- **Memory**: ~100KB for 9000 nodes graph

### File Format (CSV)

**city-locations.csv**:
```csv
id,x,y,type,zone
zone4_township-B7_S6_N9,2304,-1380,route,zone4
zone4_township-B7_S6_Loc9,2304,-1400,home,zone4
```

**paths.csv**:
```csv
from,to,distance
zone4_township-B7_S6_N9,zone4_township-B7_S6_N8,22
zone4_township-B7_S6_N8,zone4_township-B7_S6_N7,22
```

---

## 📊 Test Results

### Complete System Test
- ✅ **City Graph**: 9076 nodes, 10027 edges loaded
- ✅ **Driver Management**: Creation, assignment, location updates
- ✅ **Trip Lifecycle**: All 6 states tested
- ✅ **Pathfinding**: 5648m cross-zone route calculated
- ✅ **Real-Time Tracking**: 75 movement steps
- ✅ **Payment**: 847.2 base + 100 surcharge = 947.2 Rupees
- ✅ **Timing**: 2-second delays working correctly

### Performance Metrics
- **Path Calculation**: <100ms for 75-node path
- **Memory Usage**: ~15MB for full system
- **CSV Loading**: ~1 second for 9000+ nodes

---

## 🏆 Features Summary

| Feature | Status | Description |
|---------|--------|-------------|
| Graph Loading | ✅ Complete | CSV parsing, dynamic node/edge creation |
| A* Pathfinding | ✅ Complete | Optimal route calculation |
| Trip State Machine | ✅ Complete | 6-state lifecycle management |
| Real-Time Tracking | ✅ Complete | Location updates every 2 seconds |
| Payment System | ✅ Complete | Distance-based + cross-zone surcharge |
| Rollback System | ✅ Complete | Operation history & reversal |
| Driver Policies | ✅ Complete | Route node restriction |
| Rider Policies | ✅ Complete | Any node allowed, auto-resolution |
| Movement Simulation | ✅ Complete | Step-by-step with timing |
| Cross-Zone Support | ✅ Complete | Multi-zone routing |

---

## 📝 License

This project is created for educational purposes.

---

## 👥 Authors

Developed as a comprehensive ride-sharing system demonstration showcasing:
- Data structures (graphs, linked lists)
- Algorithms (A*, pathfinding)
- Design patterns (state machine, facade, strategy)
- Real-time simulation
- Payment systems
- Rollback mechanisms

---
## App download:
https://drive.google.com/file/d/1vGCv1SvkXcNHleOnOMxdi0MdpU_rXnp7/view?usp=sharing

**Last Updated**: January 21, 2026
