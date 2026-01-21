#ifndef ROLLBACKMANAGER_H
#define ROLLBACKMANAGER_H

#include "trip.h"
#include "driver.h"

// Snapshot of an operation for rollback
struct OperationSnapshot
{
    int operationType;  // 0: trip_assign, 1: trip_cancel, 2: trip_complete
    int tripId;
    int driverId;
    TripState previousState;
    bool driverWasAvailable;
    OperationSnapshot *next;
    
    OperationSnapshot() : operationType(-1), tripId(-1), driverId(-1), 
                         previousState(REQUESTED), driverWasAvailable(true), next(nullptr) {}
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
                       TripState state, bool driverAvail);

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

    // Display
    void displayHistory() const;
};

#endif // ROLLBACKMANAGER_H
