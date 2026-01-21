#ifndef DISPATCHENGINE_H
#define DISPATCHENGINE_H

#include "city.h"
#include "driver.h"
#include "rider.h"
#include "trip.h"
#include "rollbackmanager.h"

// Structure to hold active trip information
struct ActiveTrip
{
    Trip *trip;
    Driver *driver;
    ActiveTrip *next;
    
    ActiveTrip(Trip *t, Driver *d) : trip(t), driver(d), next(nullptr) {}
};

class DispatchEngine
{
private:
    City *city;
    Driver **drivers;       // Array of driver pointers
    int driverCount;
    int maxDrivers;
    
    Trip **trips;           // Array of trip pointers
    int tripCount;
    int maxTrips;
    
    ActiveTrip *activeTripsHead;  // Linked list of active trips
    RollbackManager *rollbackManager;  // Integrated rollback system
    
    // Helper methods
    int findNearestAvailableDriver(const char *pickupNodeId, bool sameZone);
    Driver *selectBestDriver(Driver **candidates, int count, 
                           const char *pickupNodeId, bool sameZonePref);
    void addActiveTrip(Trip *trip, Driver *driver);
    void removeActiveTrip(int tripId);
    ActiveTrip *findActiveTrip(int tripId);
    
    // NEW: Location and validation helpers
    const char *resolveRiderPickupNode(const char *riderNodeId);
    bool validateDriverNode(const char *nodeId) const;
    const char *findNearestRouteNode(double x, double y) const;

public:
    DispatchEngine(City *c, int maxD = 50, int maxT = 100);
    ~DispatchEngine();

    // Driver management
    bool addDriver(int driverId, const char *nodeId, const char *zone);
    bool removeDriver(int driverId);
    Driver *getDriver(int driverId) const;

    // Trip management
    bool requestTrip(int tripId, int riderId, const char *pickupNodeId, 
                    const char *dropoffNodeId);
    bool assignTrip(int tripId, int driverId);
    bool startTrip(int tripId);
    bool completeTrip(int tripId);
    bool cancelTrip(int tripId);

    // Queries
    int getTripCount() const;
    Trip *getTrip(int tripId) const;
    ActiveTrip *getActiveTripsHead() const;
    int getActiveTripsCount() const;
    
    // Movement simulation
    bool startPickupMovement(int tripId);
    bool advanceTripMovement(int tripId);  // Advances one step, returns true if more remain
    
    // Rollback access
    RollbackManager *getRollbackManager() const;
    
    // Display
    void displayDrivers() const;
    void displayTrips() const;
    void displayActiveTrips() const;
};

#endif // DISPATCHENGINE_H
