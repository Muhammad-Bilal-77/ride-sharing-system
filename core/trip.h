#ifndef TRIP_H
#define TRIP_H

#include "city.h"

enum TripState
{
    REQUESTED,
    ASSIGNED,
    ONGOING,
    COMPLETED,
    CANCELLED
};

class Trip
{
private:
    int tripId;
    int riderId;
    int driverId;
    TripState state;
    char pickupNodeId[MAX_STRING_LENGTH];
    char dropoffNodeId[MAX_STRING_LENGTH];
    PathResult driverToPickupPath;
    PathResult pickupToDropoffPath;

public:
    Trip(int id, int rider, const char *pickup, const char *dropoff);
    ~Trip();

    // Getters
    int getTripId() const;
    int getRiderId() const;
    int getDriverId() const;
    TripState getState() const;
    const char *getPickupNodeId() const;
    const char *getDropoffNodeId() const;
    const PathResult &getDriverToPickupPath() const;
    const PathResult &getPickupToDropoffPath() const;
    double getTotalDistance() const;

    // State transitions
    bool transitionToAssigned(int driver);
    bool transitionToOngoing();
    bool transitionToCompleted();
    bool transitionToCancelled();

    // Path setters
    void setDriverToPickupPath(const PathResult &path);
    void setPickupToDropoffPath(const PathResult &path);

    // State setter (for rollback)
    void setState(TripState s);

    // Validation
    static bool isValidTransition(TripState from, TripState to);

    // Display
    void display() const;
    const char *stateToString(TripState s) const;
};

#endif // TRIP_H
