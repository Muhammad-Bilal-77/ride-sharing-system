#ifndef TRIP_H
#define TRIP_H

#include "city.h"

enum TripState
{
    REQUESTED,
    ASSIGNED,
    PICKUP_IN_PROGRESS,
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
    char effectivePickupNodeId[MAX_STRING_LENGTH];  // Resolved pickup node (route node)
    char driverCurrentNodeId[MAX_STRING_LENGTH];     // Real-time driver location
    char riderCurrentNodeId[MAX_STRING_LENGTH];      // Real-time rider location
    PathResult driverToPickupPath;
    PathResult pickupToDropoffPath;
    int currentPathIndex;                            // For movement simulation

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
    const char *getEffectivePickupNodeId() const;
    const char *getDriverCurrentNodeId() const;
    const char *getRiderCurrentNodeId() const;
    const PathResult &getDriverToPickupPath() const;
    const PathResult &getPickupToDropoffPath() const;
    double getTotalDistance() const;
    double getRideDistance() const;              // Distance rider actually travels (pickup->dropoff)
    int getCurrentPathIndex() const;

    // State transitions
    bool transitionToAssigned(int driver);
    bool transitionToPickupInProgress();
    bool transitionToOngoing();
    bool transitionToCompleted();
    bool transitionToCancelled();

    // Path setters
    void setDriverToPickupPath(const PathResult &path);
    void setPickupToDropoffPath(const PathResult &path);
    void setEffectivePickupNodeId(const char *nodeId);

    // State setter (for rollback)
    void setState(TripState s);

    // Location setters (for movement simulation)
    void setDriverCurrentNodeId(const char *nodeId);
    void setRiderCurrentNodeId(const char *nodeId);
    void setCurrentPathIndex(int index);

    // Movement simulation
    bool advanceMovement();  // Advances one step, returns true if more steps remain

    // Validation
    static bool isValidTransition(TripState from, TripState to);

    // Payment calculation
    double calculateBaseFare() const;        // Calculate fare based on distance (150 rupees per 1000m)
    double calculateZoneSurcharge() const;   // Check if cross-zone and add 100 rupees surcharge
    double calculateTotalFare() const;       // Total fare including surcharge
    
    // Helper function to extract zone from node ID
    static void extractZone(const char *nodeId, char *zone, int maxLen);

    // Display
    void display() const;
    const char *stateToString(TripState s) const;
};

#endif // TRIP_H
