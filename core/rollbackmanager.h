#ifndef ROLLBACKMANAGER_H
#define ROLLBACKMANAGER_H

#include "trip.h"
#include "driver.h"

// Snapshot of an operation for rollback
struct OperationSnapshot
{
    int operationType;  // 0: trip_assign, 1: trip_cancel, 2: trip_complete, 3: rider_location_change, 4: driver_availability_change
    int tripId;
    int riderId;
    int driverId;
    TripState previousState;
    bool driverWasAvailable;   // availability before the change
    bool driverNewAvailable;   // availability after the change (for opType 4)
    char driverLocation[64];  // Store driver's current location node
    char riderLocation[64];   // Store rider's current location node
    // Extended fields for richer snapshot (e.g., trip history)
    char details[128];        // status/note for display (e.g., COMPLETED/CANCELLED)
    char pickup[64];
    char dropoff[64];
    double fare;
    double distance;
    char riderCode[16];       // e.g., R07
    OperationSnapshot *next;
    
    OperationSnapshot() : operationType(-1), tripId(-1), riderId(-1), driverId(-1), 
                         previousState(REQUESTED), driverWasAvailable(true), driverNewAvailable(true), fare(0.0), distance(0.0), next(nullptr) {
        driverLocation[0] = '\0';
        riderLocation[0] = '\0';
        details[0] = '\0';
        pickup[0] = '\0';
        dropoff[0] = '\0';
        riderCode[0] = '\0';
    }
};

class RollbackManager
{
private:
    OperationSnapshot *snapshotStack;
    int operationCount;
    int maxOperations;

public:
    RollbackManager(int maxOps = 100);
    ~RollbackManager();

    // Record operation before it happens
    void recordSnapshot(int opType, int tripId, int driverId, 
                       TripState state, bool driverAvail, const char *driverLoc = nullptr,
                       int riderId = -1, const char *riderLoc = nullptr,
                       bool driverNewAvail = true, const char *riderCode = nullptr);

    // Record a trip history snapshot (for rollback view/analytics)
    void recordHistorySnapshot(int tripId, int riderId, int driverId,
                               const char *pickup, const char *dropoff,
                               const char *status, double fare, double distance,
                               const char *riderCode);

    // Rollback last operation
    bool rollbackLast(Trip **trips, int tripCount, 
                     class DispatchEngine *engine);

    // Rollback last k operations
    bool rollbackLastK(int k, Trip **trips, int tripCount,
                      class DispatchEngine *engine);

    // Clear history
    void clearHistory();

    // Queries
    int getOperationCount() const;
    bool canRollback() const;
    OperationSnapshot *getSnapshotStack() const { return snapshotStack; }

    // Display
    void displayHistory() const;
};

#endif // ROLLBACKMANAGER_H
