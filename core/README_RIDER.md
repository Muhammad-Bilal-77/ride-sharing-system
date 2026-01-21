# Rider Management Component

## 📋 Overview

The Rider component represents passengers in the ride-sharing system who can request trips from any location (route nodes or residential areas) to any destination.

---

## 🎯 Purpose

- Represent individual riders (passengers)
- Store rider identification information
- Enable location flexibility (any node type)
- Track rider's current trip status
- Support trip request creation

---

## 🏗️ Data Structure

```cpp
class Rider {
private:
    int riderId;                    // Unique identifier
    char name[MAX_STRING_LENGTH];   // Rider name
    char phoneNumber[MAX_STRING_LENGTH];  // Contact information

public:
    // Constructor
    Rider(int id, const char *riderName, const char *phone);
    
    // Getters
    int getRiderId() const;
    const char *getName() const;
    const char *getPhoneNumber() const;
};
```

---

## 🌍 Location Flexibility

### Policy
Riders can be at **ANY** location type - route nodes OR residential locations.

### Rationale
- **Real-World Accuracy**: Passengers are at homes/offices (residential locations)
- **Pickup Resolution**: System resolves residential → nearest route node
- **Dropoff Support**: Riders can request any destination
- **Flexibility**: No artificial constraints on pickup/dropoff

### Location Types Supported

#### 1. Residential Locations (Primary Use Case)
```
Pattern: zone{N}_{area}-B{X}_S{Y}_Loc{Z}
Examples:
  - zone4_township-B7_S6_Loc9       ← Home address
  - zone3_johar_town-B10_S2_Loc15   ← Office
  - zone2_gulberg-B5_S3_Loc22       ← Shopping center
```

#### 2. Route Nodes (Also Valid)
```
Pattern: zone{N}_{area}-B{X}_S{Y}_N{Z}
Examples:
  - zone4_township-B7_S6_N9         ← Street intersection
  - zone3_johar_town-B10_S2_N15     ← Main road
  - zone2_gulberg-B5_S3_N22         ← Junction
```

---

## 🔑 Key Methods

### Constructor

#### `Rider(int id, const char *riderName, const char *phone)`
**Purpose**: Create new rider profile

**Parameters**:
- `id`: Unique rider ID
- `riderName`: Rider's name (max 255 chars)
- `phone`: Contact phone number (max 255 chars)

**Example**:
```cpp
Rider rider(101, "Fatima Ahmed", "+92-300-1234567");
```

### Getters

#### `int getRiderId() const`
**Returns**: Unique rider identifier

#### `const char *getName() const`
**Returns**: Rider's name

#### `const char *getPhoneNumber() const`
**Returns**: Contact phone number

---

## 📊 Data Flow

```
┌─────────────────┐
│  Create Rider   │ ← Constructor(id, name, phone)
│  Profile        │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ Request Trip    │ ← System creates Trip object
│ (Any Location)  │   Pickup: residential or route node
└────────┬────────┘   Dropoff: residential or route node
         │
         ▼
┌─────────────────┐
│ Pickup          │ ← Driver arrives at resolved location
│ Resolution      │   DispatchEngine finds nearest route node
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ In Transit      │ ← Trip ongoing, rider with driver
│                 │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ Dropoff         │ ← Trip completed at destination
│ Complete        │   Rider reaches final location
└─────────────────┘
```

---

## 🔧 Usage Examples

### Example 1: Create Rider Profile

```cpp
// Create rider
Rider rider(101, "Muhammad Hassan", "+92-321-9876543");

// Display info
std::cout << "Rider ID: " << rider.getRiderId() << std::endl;
std::cout << "Name: " << rider.getName() << std::endl;
std::cout << "Phone: " << rider.getPhoneNumber() << std::endl;

// Output:
// Rider ID: 101
// Name: Muhammad Hassan
// Phone: +92-321-9876543
```

### Example 2: Request Trip from Home (Residential)

```cpp
Rider rider(101, "Sarah Khan", "+92-300-1111111");

// Pickup: Residential location (home)
const char *pickup = "zone4_township-B7_S6_Loc9";

// Dropoff: Residential location (office)
const char *dropoff = "zone3_johar_town-B7_S6_Loc9";

// Request trip
Trip trip(1, rider.getRiderId(), pickup, dropoff);

// System resolves:
// - Pickup resolves to: zone4_township-B7_S6_N9 (nearest route node)
// - Dropoff resolves to: zone3_johar_town-B7_S6_N9 (nearest route node)
```

### Example 3: Request Trip from Intersection (Route Node)

```cpp
Rider rider(102, "Ahmed Ali", "+92-333-2222222");

// Pickup: Already at route node (street corner)
const char *pickup = "zone3_johar_town-B10_S2_N15";

// Dropoff: Residential destination
const char *dropoff = "zone2_gulberg-B5_S3_Loc22";

// Request trip
Trip trip(2, rider.getRiderId(), pickup, dropoff);

// System resolves:
// - Pickup: No resolution needed (already route node)
// - Dropoff resolves to: zone2_gulberg-B5_S3_N22
```

### Example 4: Multiple Trips for Same Rider

```cpp
Rider rider(103, "Zainab Malik", "+92-345-3333333");

// Trip 1: Morning commute (home → office)
Trip trip1(1, rider.getRiderId(), 
    "zone4_township-B7_S6_Loc9",        // Home
    "zone3_johar_town-B10_S2_Loc15"     // Office
);

// Trip 2: Lunch break (office → restaurant)
Trip trip2(2, rider.getRiderId(),
    "zone3_johar_town-B10_S2_Loc15",    // Office
    "zone2_gulberg-B5_S3_Loc22"         // Restaurant
);

// Trip 3: Evening return (office → home)
Trip trip3(3, rider.getRiderId(),
    "zone3_johar_town-B10_S2_Loc15",    // Office
    "zone4_township-B7_S6_Loc9"         // Home
);

// Same rider, different trips
assert(trip1.getRiderId() == trip2.getRiderId());
assert(trip2.getRiderId() == trip3.getRiderId());
```

---

## 🆚 Rider vs Driver Locations

| Aspect | Driver | Rider |
|--------|--------|-------|
| **Allowed Locations** | Route nodes ONLY | Any location |
| **Location Pattern** | `zone{N}_{area}-B{X}_S{Y}_N{Z}` | `N{Z}` OR `Loc{Z}` |
| **Rationale** | Must be routable | Real-world flexibility |
| **Pickup Resolution** | N/A (always valid) | System resolves to route node |
| **Example Valid** | `zone4_township-B7_S6_N9` | `zone4_township-B7_S6_Loc9` |
| **Example Invalid** | `zone4_township-B7_S6_Loc9` ❌ | None (all allowed) ✅ |

---

## 🔍 Pickup Resolution Process

When rider requests pickup from residential location:

```
1. Rider Location (Residential)
   └─ "zone4_township-B7_S6_Loc9"

2. System Query
   └─ city.findNearestRouteNode("zone4_township-B7_S6_Loc9")

3. Resolved Pickup (Route Node)
   └─ "zone4_township-B7_S6_N9"

4. Trip Updated
   └─ effectivePickupNodeId = "zone4_township-B7_S6_N9"
   └─ Original pickupNodeId = "zone4_township-B7_S6_Loc9" (preserved)

5. Driver Routes To
   └─ Effective pickup: zone4_township-B7_S6_N9
```

**Note**: Original residential location preserved for reference, but driver navigates to resolved route node.

---

## 💡 Design Decisions

### Why No Location Tracking for Riders?

**Current**: Rider object stores only profile (ID, name, phone)
- Location specified per trip request
- Different pickup for each trip
- Flexibility in trip creation

**Alternative**: Store `currentLocation` field
```cpp
char currentLocation[MAX_STRING_LENGTH];  // Not implemented
```

**Rejected Because**:
- Riders mobile, location changes frequently
- Trip-specific location more accurate
- Reduces coupling between Rider and Trip

### Why Immutable Profile?

**Current**: No setters for name or phone
- Profile established at creation
- Contact info rarely changes
- Simpler state management

**Future Extension**: Add update methods
```cpp
void updateName(const char *newName);
void updatePhoneNumber(const char *newPhone);
```

---

## 📈 Rider Information

| Field | Type | Purpose | Example |
|-------|------|---------|---------|
| riderId | int | Unique identifier | 101 |
| name | char[256] | Full name | "Muhammad Hassan" |
| phoneNumber | char[256] | Contact | "+92-321-9876543" |

---

## 🧪 Integration Points

### With Trip
```cpp
// Rider requests trip
Rider rider(101, "Ali", "+92-300-1234567");
Trip trip(1, rider.getRiderId(), pickup, dropoff);

// Verify rider
assert(trip.getRiderId() == rider.getRiderId());
```

### With DispatchEngine
```cpp
// Create trip for rider
Trip *trip = dispatchEngine.createTrip(
    rider.getRiderId(),
    "zone4_township-B7_S6_Loc9",  // Residential pickup
    "zone3_johar_town-B7_S6_Loc9"  // Residential dropoff
);

// System resolves residential → route nodes internally
```

### With RideShareSystem
```cpp
// High-level API
system.addRider(rider.getRiderId(), rider.getName(), rider.getPhoneNumber());
system.requestTrip(
    rider.getRiderId(),
    "zone4_township-B7_S6_Loc9",
    "zone3_johar_town-B7_S6_Loc9"
);
```

---

## ⚙️ Implementation Details

### String Handling
- Uses `strncpy()` for safe copying
- MAX_STRING_LENGTH = 256 bytes
- Null-termination guaranteed

### Memory Layout
```cpp
sizeof(Rider) = 4 + 256 + 256 = 516 bytes
                │   │     └─ phoneNumber (char[256])
                │   └─────── name (char[256])
                └───────────── riderId (int)
```

### Phone Number Format
- No validation (accepts any string)
- Typical format: "+92-XXX-XXXXXXX"
- Could add validation in future:
  ```cpp
  bool isValidPhoneNumber(const char *phone);
  ```

---

## 📦 Dependencies

- `<cstring>`: String operations (strncpy, strlen)
- `city.h`: MAX_STRING_LENGTH constant

---

## 🐛 Error Handling

- **Null Name**: No validation (assumes valid input)
- **Empty Phone**: No validation (assumes valid input)
- **Duplicate ID**: No check (assumes system manages uniqueness)

**Note**: Input validation delegated to higher-level components (RideShareSystem)

---

## 📊 Performance

| Operation | Complexity | Notes |
|-----------|------------|-------|
| Constructor | O(n + m) | String copies (n=name, m=phone) |
| getRiderId() | O(1) | Direct field access |
| getName() | O(1) | Returns pointer |
| getPhoneNumber() | O(1) | Returns pointer |

---

## 🔄 Typical Lifecycle

```cpp
// 1. Create rider profile
Rider rider(101, "Fatima Ahmed", "+92-321-1111111");

// 2. Add to system
system.addRider(rider.getRiderId(), rider.getName(), rider.getPhoneNumber());

// 3. Request trip #1 (morning commute)
system.requestTrip(
    rider.getRiderId(),
    "zone4_township-B7_S6_Loc9",      // Home
    "zone3_johar_town-B10_S2_Loc15"   // Office
);

// 4. Trip assigned and completed
// ...

// 5. Request trip #2 (evening return)
system.requestTrip(
    rider.getRiderId(),
    "zone3_johar_town-B10_S2_Loc15",  // Office
    "zone4_township-B7_S6_Loc9"       // Home
);

// 6. Trip assigned and completed
// ...

// Rider profile persists across multiple trips
```

---

## 🌟 Real-World Scenarios

### Scenario 1: Residential Pickup/Dropoff
```cpp
// Most common case
Rider rider(101, "Ali", "+92-300-1234567");

// Both locations residential
Trip trip(1, rider.getRiderId(),
    "zone4_township-B7_S6_Loc9",    // Home (residential)
    "zone3_johar_town-B7_S6_Loc9"   // Friend's house (residential)
);

// System handles:
// 1. Resolve pickup: Loc9 → N9 (nearest route node)
// 2. Resolve dropoff: Loc9 → N9 (nearest route node)
// 3. Calculate path: N9 → ... → N9
// 4. Driver picks up at: zone4_township-B7_S6_N9
// 5. Driver drops off at: zone3_johar_town-B7_S6_N9
```

### Scenario 2: Mixed Locations
```cpp
// Rider at intersection, going home
Trip trip(2, rider.getRiderId(),
    "zone3_johar_town-B10_S2_N15",  // Route node (no resolution)
    "zone4_township-B7_S6_Loc9"     // Home (resolve to N9)
);
```

### Scenario 3: Cross-Zone Trip
```cpp
// Long distance trip with zone change
Trip trip(3, rider.getRiderId(),
    "zone4_township-B7_S6_Loc9",    // Zone 4
    "zone2_gulberg-B5_S3_Loc22"     // Zone 2
);

// Payment includes:
// - Base fare: Distance-based
// - Surcharge: +100 Rupees (cross-zone)
```

---

## 📝 Notes

- **Profile Immutability**: Once created, rider profile cannot be modified
- **Location Independence**: Rider location not stored in object (trip-specific)
- **Phone Flexibility**: No format validation (accepts any string)
- **Multiple Trips**: Same rider can have many trips (different Trip objects)
- **Contact Info**: Phone number used for notifications (future feature)

---

**File**: `core/rider.h`, `core/rider.cpp`  
**Last Updated**: January 21, 2026
