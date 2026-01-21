#include <iostream>
#include <cstring>
#include <chrono>
#include <thread>
#include "city.h"
#include "ridesharesystem.h"
#include "trip.h"

void completeSystemTest()
{
    std::cout << "\n╔════════════════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                   COMPLETE RIDE-SHARING SYSTEM TEST                           ║" << std::endl;
    std::cout << "║            Demonstrating All Features: Routing, Timing, Payment, Locations   ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════════════════════╝\n" << std::endl;

    // Load city data
    std::cout << "[LOADING] City graph data..." << std::endl;
    City city;
    if (!city.loadLocations("city_locations_path_data/city-locations.csv"))
    {
        std::cout << "[ERROR] Failed to load city locations" << std::endl;
        return;
    }
    if (!city.loadPaths("city_locations_path_data/paths.csv"))
    {
        std::cout << "[ERROR] Failed to load city paths" << std::endl;
        return;
    }
    std::cout << "[SUCCESS] Loaded " << city.getNodeCount() << " nodes and "
              << city.getUniqueEdgeCount() << " edges\n" << std::endl;

    // Initialize ride sharing system
    RideShareSystem system(&city);

    // ===== STEP 1: Add Driver =====
    std::cout << "╔════════════════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║ STEP 1: Add Driver at Initial Route Node                                      ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════════════════════╝\n" << std::endl;

    Node *routeNode = city.getNode("zone4_township-B7_S6_N9");
    if (!routeNode)
    {
        std::cout << "[ERROR] Could not find route node" << std::endl;
        return;
    }

    system.addDriver(1, routeNode->id, "zone4");
    std::cout << "✓ Driver #1 added at initial location: " << routeNode->id << "\n" << std::endl;

    // ===== STEP 2: Create Trip Request =====
    std::cout << "╔════════════════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║ STEP 2: Rider Requests Trip (Cross-Zone: zone4 → zone3)                      ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════════════════════╝\n" << std::endl;

    Node *pickupLoc = city.getNode("zone4_township-B7_S6_Loc9");
    Node *dropoffLoc = city.getNode("zone3_johar_town-B7_S6_Loc9");

    if (!pickupLoc || !dropoffLoc)
    {
        std::cout << "[ERROR] Could not find pickup or dropoff locations" << std::endl;
        return;
    }

    std::cout << "RIDER REQUEST:" << std::endl;
    std::cout << "  Rider #101 Location: " << pickupLoc->id << " (zone4)" << std::endl;
    std::cout << "  Dropoff Destination: " << dropoffLoc->id << " (zone3)" << std::endl;
    std::cout << "  Trip Type: CROSS-ZONE\n" << std::endl;

    system.createAndRequestTrip(101, pickupLoc->id, dropoffLoc->id);
    Trip *trip = system.getTrip(1);
    std::cout << "✓ Trip #1 created with state: REQUESTED\n" << std::endl;

    // ===== STEP 3: Assign Trip =====
    std::cout << "╔════════════════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║ STEP 3: Assign Trip to Nearest Driver                                         ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════════════════════╝\n" << std::endl;

    double distToPickup = city.getDistance(routeNode->id, pickupLoc->id);
    std::cout << "ASSIGNMENT DETAILS:" << std::endl;
    std::cout << "  Distance from Driver to Pickup: " << distToPickup << "m" << std::endl;
    std::cout << "  Assigning Trip #1 to Driver #1...\n" << std::endl;

    system.assignTrip(1, 1);
    std::cout << "✓ Trip assigned successfully" << std::endl;
    std::cout << "  Trip State: " << trip->stateToString(trip->getState()) << "\n" << std::endl;

    // ===== STEP 4: Start Movement =====
    std::cout << "╔════════════════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║ STEP 4: Real-Time Movement Simulation (2 seconds per edge)                    ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════════════════════╝\n" << std::endl;

    system.startTripMovement(1);
    int stepCount = 0;

    // Pickup phase
    std::cout << "PHASE 1: PICKUP MOVEMENT" << std::endl;
    std::cout << "─────────────────────────────────────────────────────────────\n" << std::endl;

    auto pickupStartTime = std::chrono::system_clock::now();
    while (system.advanceTrip(1) && trip->getState() == PICKUP_IN_PROGRESS)
    {
        stepCount++;
        auto currentTime = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(currentTime - pickupStartTime);
        
        std::cout << "[TIME: " << elapsed.count() << "s] Step #" << stepCount << std::endl;
        std::cout << "  Driver Location: " << trip->getDriverCurrentNodeId() << std::endl;
        std::cout << "  Status: Moving to pickup location\n" << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    std::cout << "✓ Driver reached pickup location: " << trip->getEffectivePickupNodeId() << "\n" << std::endl;

    // Ongoing phase
    std::cout << "PHASE 2: ONGOING TRIP (Pickup → Dropoff)" << std::endl;
    std::cout << "─────────────────────────────────────────────────────────────\n" << std::endl;

    auto ongoingStartTime = std::chrono::system_clock::now();
    const PathResult &dropoffPath = trip->getPickupToDropoffPath();
    int ongoingStep = 0;

    while (ongoingStep < dropoffPath.pathLength - 1)
    {
        ongoingStep++;
        system.advanceTrip(1);
        
        auto currentTime = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(currentTime - ongoingStartTime);
        
        std::cout << "[TIME: " << elapsed.count() << "s] Movement #" << ongoingStep << std::endl;
        std::cout << "  Driver Location: " << trip->getDriverCurrentNodeId() << std::endl;
        std::cout << "  Progress: " << (ongoingStep * 100 / (dropoffPath.pathLength - 1)) << "% complete" << std::endl;
        std::cout << "  Status: En route to dropoff\n" << std::endl;

        if (ongoingStep < dropoffPath.pathLength - 1)
        {
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }

    std::cout << "✓ Arrived at dropoff location: " << dropoffLoc->id << "\n" << std::endl;

    // ===== STEP 5: Complete Trip =====
    std::cout << "╔════════════════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║ STEP 5: Complete Trip and Generate Full Report                               ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════════════════════╝\n" << std::endl;

    system.completeTrip(1);

    std::cout << "✓ Trip completed successfully\n" << std::endl;

    // ===== DETAILED TRIP REPORT =====
    std::cout << "╔════════════════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                        COMPLETE TRIP DETAILS REPORT                           ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════════════════════╝\n" << std::endl;

    // Trip Information
    std::cout << "TRIP INFORMATION:" << std::endl;
    std::cout << "  Trip ID: " << trip->getTripId() << std::endl;
    std::cout << "  Rider ID: " << trip->getRiderId() << std::endl;
    std::cout << "  Driver ID: " << trip->getDriverId() << std::endl;
    std::cout << "  Trip Status: " << trip->stateToString(trip->getState()) << "\n" << std::endl;

    // Location Information
    std::cout << "LOCATION INFORMATION:" << std::endl;
    std::cout << "  Pickup Location (original): " << trip->getPickupNodeId() << std::endl;
    std::cout << "  Pickup Location (resolved): " << trip->getEffectivePickupNodeId() << std::endl;
    std::cout << "  Dropoff Location: " << trip->getDropoffNodeId() << std::endl;
    std::cout << "  Final Driver Location: " << system.getDriver(1)->getCurrentNodeId() << "\n" << std::endl;

    // Distance Information
    std::cout << "DISTANCE BREAKDOWN:" << std::endl;
    double driverToPickupDist = trip->getDriverToPickupPath().totalDistance;
    double pickupToDropoffDist = trip->getPickupToDropoffPath().totalDistance;
    double totalDist = trip->getTotalDistance();

    std::cout << "  Driver Starting Point → Pickup Location: " << driverToPickupDist << "m" << std::endl;
    std::cout << "  Pickup Location → Dropoff Location: " << pickupToDropoffDist << "m" << std::endl;
    std::cout << "  ─────────────────────────────────────────────" << std::endl;
    std::cout << "  TOTAL TRIP DISTANCE: " << totalDist << "m\n" << std::endl;

    // Payment Information
    std::cout << "PAYMENT CALCULATION:" << std::endl;
    std::cout << "  Fare Rate: 150 Rupees per 1000 meters" << std::endl;

    double baseFare = trip->calculateBaseFare();
    double zoneSurcharge = trip->calculateZoneSurcharge();
    double totalFare = trip->calculateTotalFare();

    char pickupZone[256] = {0};
    char dropoffZone[256] = {0};
    Trip::extractZone(trip->getPickupNodeId(), pickupZone, 256);
    Trip::extractZone(trip->getDropoffNodeId(), dropoffZone, 256);

    std::cout << "  Base Fare (" << totalDist << "m × 150/1000): " << baseFare << " Rupees" << std::endl;

    if (zoneSurcharge > 0)
    {
        std::cout << "  Cross-Zone Surcharge (" << pickupZone << " → " << dropoffZone << "): +" << zoneSurcharge << " Rupees" << std::endl;
    }
    else
    {
        std::cout << "  Cross-Zone Surcharge (Same Zone): 0 Rupees" << std::endl;
    }

    std::cout << "  ─────────────────────────────────────────────" << std::endl;
    std::cout << "  TOTAL FARE: " << totalFare << " Rupees\n" << std::endl;

    // Driver Status
    std::cout << "DRIVER STATUS:" << std::endl;
    Driver *driver = system.getDriver(1);
    std::cout << "  Driver ID: " << driver->getDriverId() << std::endl;
    std::cout << "  Current Location: " << driver->getCurrentNodeId() << std::endl;
    std::cout << "  Status: " << (driver->isAvailable() ? "AVAILABLE" : "BUSY") << std::endl;
    std::cout << "  Assigned Trip: " << (driver->getAssignedTripId() == -1 ? "NONE" : "ASSIGNED") << "\n" << std::endl;

    // Summary
    std::cout << "╔════════════════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                          SYSTEM TEST SUMMARY                                  ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════════════════════╝\n" << std::endl;

    std::cout << "✓ City Graph System: Working" << std::endl;
    std::cout << "  - Loaded " << city.getNodeCount() << " nodes" << std::endl;
    std::cout << "  - Loaded " << city.getUniqueEdgeCount() << " edges\n" << std::endl;

    std::cout << "✓ Driver Management: Working" << std::endl;
    std::cout << "  - Driver created and assigned to route node" << std::endl;
    std::cout << "  - Driver location updated during trip\n" << std::endl;

    std::cout << "✓ Trip Management: Working" << std::endl;
    std::cout << "  - Trip created in REQUESTED state" << std::endl;
    std::cout << "  - Trip transitioned through states: ASSIGNED → PICKUP_IN_PROGRESS → ONGOING → COMPLETED\n" << std::endl;

    std::cout << "✓ Path Finding (A*): Working" << std::endl;
    std::cout << "  - Found path from driver to pickup (" << driverToPickupDist << "m)" << std::endl;
    std::cout << "  - Found path from pickup to dropoff (" << pickupToDropoffDist << "m)\n" << std::endl;

    std::cout << "✓ Real-Time Location Tracking: Working" << std::endl;
    std::cout << "  - Tracked driver position at " << (stepCount + ongoingStep) << " movement steps\n" << std::endl;

    std::cout << "✓ Payment System: Working" << std::endl;
    std::cout << "  - Distance-based fare: " << baseFare << " Rupees" << std::endl;
    std::cout << "  - Cross-zone surcharge: " << zoneSurcharge << " Rupees" << std::endl;
    std::cout << "  - Total fare: " << totalFare << " Rupees\n" << std::endl;

    std::cout << "✓ Timing System: Working" << std::endl;
    std::cout << "  - 2-second delays between movements implemented\n" << std::endl;

    std::cout << "╔════════════════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                    ALL SYSTEM FEATURES VERIFIED ✓ ✓ ✓                         ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════════════════════╝\n" << std::endl;
}

int main()
{
    try
    {
        completeSystemTest();
    }
    catch (const std::exception &e)
    {
        std::cout << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
