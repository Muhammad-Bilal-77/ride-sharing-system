#ifndef RIDESHARESYSTEM_H
#define RIDESHARESYSTEM_H

#include "city.h"
#include "dispatchengine.h"

// Analytics data structure (no STL)
struct AnalyticsData
{
    int totalTrips;
    int completedTrips;
    int cancelledTrips;
    double totalDistance;
    int driverCount;
    
    AnalyticsData() : totalTrips(0), completedTrips(0), cancelledTrips(0),
                     totalDistance(0.0), driverCount(0) {}
};

class RideShareSystem
{
private:
    City *city;
    DispatchEngine *dispatchEngine;
    int nextTripId;
    int nextRiderId;

public:
    RideShareSystem(City *c);
    ~RideShareSystem();

    // Driver operations
    bool addDriver(int driverId, const char *nodeId, const char *zone);

    // Rider operations
    bool createAndRequestTrip(int riderId, const char *pickupNodeId,
                             const char *dropoffNodeId);

    // Trip operations
    bool assignTrip(int tripId, int driverId);
    bool startTrip(int tripId);
    bool completeTrip(int tripId);
    bool cancelTrip(int tripId);

    // Movement simulation
    bool startTripMovement(int tripId);  // Start pickup phase
    bool advanceTrip(int tripId);        // Advance 1 step (1 second)

    // Rollback operations (delegates to DispatchEngine)
    bool rollbackLastOperation();
    bool rollbackLastKOperations(int k);

    // Analytics
    AnalyticsData getAnalytics() const;
    double getAverageTripDistance() const;
    double getDriverUtilizationPercentage() const;

    // Queries
    Trip *getTrip(int tripId) const;
    Driver *getDriver(int driverId) const;

    // Display
    void displaySystem() const;
    void displayAnalytics() const;
};

#endif // RIDESHARESYSTEM_H
