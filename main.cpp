#include <iostream>
#include "core/city.h"
#include "core/ridesharesystem.h"

// Helper to print test results
void printResult(int testNo, const char *label, bool ok)
{
    std::cout << "\n[TEST " << testNo << "] " << label << ": " 
              << (ok ? "PASS ✓" : "FAIL ✗") << std::endl;
}

int main()
{
    std::cout << "\n========================================" << std::endl;
    std::cout << "  RIDE SHARING SYSTEM - TEST SUITE" << std::endl;
    std::cout << "========================================" << std::endl;

    // Load city data
    City city;
    bool cityLoaded = city.loadLocations("city_locations_path_data/city-locations.csv") &&
                      city.loadPaths("city_locations_path_data/paths.csv");
    
    if (!cityLoaded)
    {
        std::cout << "ERROR: Failed to load city data" << std::endl;
        return 1;
    }

    RideShareSystem *system = new RideShareSystem(&city);
    int testNo = 1;

    // TEST 1: Add multiple drivers
    std::cout << "\n[Test 1] Adding drivers..." << std::endl;
    bool d1 = system->addDriver(101, "zone1_gulberg-M4_S1_Loc2", "zone1");
    bool d2 = system->addDriver(102, "zone1_gulberg-M4_S1_Loc9", "zone1");
    bool d3 = system->addDriver(103, "zone2_DHA-M4_S1_Loc2", "zone2");
    bool d4 = system->addDriver(104, "zone2_DHA-M4_S1_Loc5", "zone2");
    printResult(testNo++, "Add 4 drivers", d1 && d2 && d3 && d4);

    // TEST 2: Create riders and request trips
    std::cout << "\n[Test 2] Creating riders and requesting trips..." << std::endl;
    bool r1 = system->createAndRequestTrip(201, "zone1_gulberg-M4_S1_Loc3", 
                                          "zone1_gulberg-M4_S1_Loc8");
    bool r2 = system->createAndRequestTrip(202, "zone2_DHA-M4_S1_Loc3", 
                                          "zone2_DHA-M4_S1_Loc4");
    bool r3 = system->createAndRequestTrip(203, "zone1_gulberg-M4_S1_Loc5", 
                                          "zone1_gulberg-M4_S2_Loc2");
    printResult(testNo++, "Create 3 riders & request trips", r1 && r2 && r3);

    // TEST 3: Basic trip assignment
    std::cout << "\n[Test 3] Assigning Trip 1 to Driver 101..." << std::endl;
    bool assign1 = system->assignTrip(1, 101);
    Trip *trip1 = system->getTrip(1);
    bool stateOk = trip1 && trip1->getState() == ASSIGNED;
    printResult(testNo++, "Trip 1 assigned (state transition)", assign1 && stateOk);

    // TEST 4: Verify A* path computation
    std::cout << "\n[Test 4] Verifying A* path computation..." << std::endl;
    bool pathOk = trip1 && trip1->getDriverToPickupPath().totalDistance > 0 &&
                  trip1->getPickupToDropoffPath().totalDistance > 0;
    printResult(testNo++, "Driver-to-pickup & pickup-to-dropoff paths computed", pathOk);
    if (pathOk)
    {
        std::cout << "  Driver->Pickup: " << trip1->getDriverToPickupPath().totalDistance << "m" << std::endl;
        std::cout << "  Pickup->Dropoff: " << trip1->getPickupToDropoffPath().totalDistance << "m" << std::endl;
    }

    // TEST 5: Start trip transition
    std::cout << "\n[Test 5] Starting Trip 1..." << std::endl;
    bool start1 = system->startTrip(1);
    bool startState = trip1 && trip1->getState() == ONGOING;
    printResult(testNo++, "Trip 1 start (ASSIGNED->ONGOING)", start1 && startState);

    // TEST 6: Complete trip transition
    std::cout << "\n[Test 6] Completing Trip 1..." << std::endl;
    bool complete1 = system->completeTrip(1);
    bool completeState = trip1 && trip1->getState() == COMPLETED;
    printResult(testNo++, "Trip 1 complete (ONGOING->COMPLETED)", complete1 && completeState);

    // TEST 7: Invalid state transition detection
    std::cout << "\n[Test 7] Testing invalid transition rejection..." << std::endl;
    bool invalidTransition = !system->completeTrip(1); // Already COMPLETED
    printResult(testNo++, "Reject invalid transition (complete twice)", invalidTransition);

    // TEST 8: Trip cancellation from REQUESTED state
    std::cout << "\n[Test 8] Cancelling Trip 2 from REQUESTED state..." << std::endl;
    bool cancel2 = system->cancelTrip(2);
    Trip *trip2 = system->getTrip(2);
    bool cancelState = trip2 && trip2->getState() == CANCELLED;
    printResult(testNo++, "Cancel Trip 2 (REQUESTED->CANCELLED)", cancel2 && cancelState);

    // TEST 9: Assign, then cancel Trip 3
    std::cout << "\n[Test 9] Assigning Trip 3, then cancelling..." << std::endl;
    bool assign3 = system->assignTrip(3, 102);
    bool cancel3 = system->cancelTrip(3);
    Trip *trip3 = system->getTrip(3);
    bool cancelAssignedState = trip3 && trip3->getState() == CANCELLED;
    Driver *d102 = system->getDriver(102);
    bool driverAvailAfterCancel = d102 && d102->isAvailable();
    printResult(testNo++, "Assign then cancel (ASSIGNED->CANCELLED) + restore driver", 
                assign3 && cancel3 && cancelAssignedState && driverAvailAfterCancel);

    // TEST 10: Single rollback operation
    std::cout << "\n[Test 10] Testing single rollback..." << std::endl;
    system->createAndRequestTrip(204, "zone1_gulberg-M4_S1_Loc2", 
                                "zone1_gulberg-M4_S1_Loc6");
    system->assignTrip(4, 103);
    Trip *trip4Before = system->getTrip(4);
    TripState stateBefore = trip4Before ? trip4Before->getState() : REQUESTED;
    Driver *d103Before = system->getDriver(103);
    bool d103AvailBefore = d103Before ? d103Before->isAvailable() : true;
    
    bool rollback = system->rollbackLastOperation();
    Trip *trip4After = system->getTrip(4);
    TripState stateAfter = trip4After ? trip4After->getState() : REQUESTED;
    Driver *d103After = system->getDriver(103);
    bool d103AvailAfter = d103After ? d103After->isAvailable() : true;
    
    bool rollbackOk = rollback && stateAfter == REQUESTED && !d103AvailBefore && d103AvailAfter;
    printResult(testNo++, "Rollback single operation", rollbackOk);

    // TEST 11: Multi-operation rollback
    std::cout << "\n[Test 11] Testing 3-operation rollback..." << std::endl;
    system->createAndRequestTrip(205, "zone2_DHA-M4_S1_Loc3", "zone2_DHA-M4_S1_Loc5");
    system->assignTrip(5, 104);
    system->startTrip(5);
    system->createAndRequestTrip(206, "zone1_gulberg-M4_S1_Loc4", "zone1_gulberg-M4_S1_Loc7");
    
    bool multiRollback = system->rollbackLastKOperations(3);
    printResult(testNo++, "Rollback 3 operations", multiRollback);

    // TEST 12: Driver availability and reassignment
    std::cout << "\n[Test 12] Testing driver availability and reassignment..." << std::endl;
    system->createAndRequestTrip(207, "zone1_gulberg-M4_S1_Loc1", 
                                "zone1_gulberg-M4_S1_Loc9");
    system->assignTrip(6, 101);
    Driver *d101 = system->getDriver(101);
    bool unavailAfterAssign = d101 && !d101->isAvailable();
    
    system->completeTrip(6);
    bool availAfterComplete = d101 && d101->isAvailable();
    printResult(testNo++, "Driver availability: unavail after assign, avail after complete", 
                unavailAfterAssign && availAfterComplete);

    // TEST 13: Analytics - average trip distance
    std::cout << "\n[Test 13] Testing analytics calculations..." << std::endl;
    AnalyticsData analytics = system->getAnalytics();
    double avgDist = system->getAverageTripDistance();
    bool analyticsOk = analytics.totalTrips > 0 && analytics.completedTrips > 0 && avgDist >= 0;
    printResult(testNo++, "Analytics: compute average distance", analyticsOk);
    std::cout << "  Total trips: " << analytics.totalTrips 
              << ", Completed: " << analytics.completedTrips 
              << ", Avg distance: " << avgDist << "m" << std::endl;

    // TEST 14: Driver utilization percentage
    std::cout << "\n[Test 14] Computing driver utilization..." << std::endl;
    double utilization = system->getDriverUtilizationPercentage();
    bool utilizationOk = utilization >= 0.0 && utilization <= 100.0;
    printResult(testNo++, "Driver utilization percentage", utilizationOk);
    std::cout << "  Utilization: " << utilization << "%" << std::endl;

    // TEST 15: Cancelled vs completed trip count
    std::cout << "\n[Test 15] Cancelled vs Completed trip analysis..." << std::endl;
    int completed = analytics.completedTrips;
    int cancelled = analytics.cancelledTrips;
    bool countOk = (completed + cancelled) <= analytics.totalTrips;
    printResult(testNo++, "Cancelled + Completed <= Total trips", countOk);
    std::cout << "  Completed: " << completed << ", Cancelled: " << cancelled 
              << ", Total: " << analytics.totalTrips << std::endl;

    // Display final system state
    system->displaySystem();
    system->displayAnalytics();

    // Cleanup
    delete system;

    std::cout << "\n========================================" << std::endl;
    std::cout << "  ALL TESTS COMPLETED" << std::endl;
    std::cout << "========================================\n" << std::endl;

    return 0;
}
