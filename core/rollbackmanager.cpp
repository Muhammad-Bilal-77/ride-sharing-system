#include "rollbackmanager.h"
#include "dispatchengine.h"
#include <iostream>
#include <cstring>

RollbackManager::RollbackManager(int maxOps)
    : snapshotStack(nullptr), operationCount(0), maxOperations(maxOps)
{
}

RollbackManager::~RollbackManager()
{
    clearHistory();
}

void RollbackManager::recordSnapshot(int opType, int tripId, int driverId,
                                    TripState state, bool driverAvail, const char *driverLoc,
                                    int riderId, const char *riderLoc, bool driverNewAvail, const char *riderCode)
{
    if (operationCount >= maxOperations)
        return;

    OperationSnapshot *snap = new OperationSnapshot();
    snap->operationType = opType;
    snap->tripId = tripId;
    snap->riderId = riderId;
    snap->driverId = driverId;
    snap->previousState = state;
    snap->driverWasAvailable = driverAvail;
    snap->driverNewAvailable = driverNewAvail;
    
    // Store driver location if provided
    if (driverLoc) {
        strncpy(snap->driverLocation, driverLoc, sizeof(snap->driverLocation) - 1);
        snap->driverLocation[sizeof(snap->driverLocation) - 1] = '\0';
    }
    
    // Store rider location if provided
    if (riderLoc) {
        strncpy(snap->riderLocation, riderLoc, sizeof(snap->riderLocation) - 1);
        snap->riderLocation[sizeof(snap->riderLocation) - 1] = '\0';
    }
    
    // Store rider code if provided
    if (riderCode) {
        strncpy(snap->riderCode, riderCode, sizeof(snap->riderCode) - 1);
        snap->riderCode[sizeof(snap->riderCode) - 1] = '\0';
    }
    
    snap->next = snapshotStack;
    
    snapshotStack = snap;
    operationCount++;
}

void RollbackManager::recordHistorySnapshot(int tripId, int riderId, int driverId,
                                            const char *pickup, const char *dropoff,
                                            const char *status, double fare, double distance,
                                            const char *riderCode)
{
    if (operationCount >= maxOperations)
        return;

    OperationSnapshot *snap = new OperationSnapshot();
    snap->operationType = 20; // TRIP_HISTORY_ENTRY
    snap->tripId = tripId;
    snap->riderId = riderId;
    snap->driverId = driverId;
    snap->previousState = COMPLETED; // indicative; actual state reflected by status
    snap->driverWasAvailable = true;
    snap->driverNewAvailable = true;

    if (status) {
        strncpy(snap->details, status, sizeof(snap->details) - 1);
        snap->details[sizeof(snap->details) - 1] = '\0';
    }
    if (pickup) {
        strncpy(snap->pickup, pickup, sizeof(snap->pickup) - 1);
        snap->pickup[sizeof(snap->pickup) - 1] = '\0';
    }
    if (dropoff) {
        strncpy(snap->dropoff, dropoff, sizeof(snap->dropoff) - 1);
        snap->dropoff[sizeof(snap->dropoff) - 1] = '\0';
    }
    snap->fare = fare;
    snap->distance = distance;
    if (riderCode) {
        strncpy(snap->riderCode, riderCode, sizeof(snap->riderCode) - 1);
        snap->riderCode[sizeof(snap->riderCode) - 1] = '\0';
    }

    snap->next = snapshotStack;
    snapshotStack = snap;
    operationCount++;
}

bool RollbackManager::rollbackLast(Trip **trips, int tripCount,
                                  DispatchEngine *engine)
{
    if (!snapshotStack || !engine)
        return false;

    OperationSnapshot *snap = snapshotStack;
    Trip *trip = nullptr;
    Driver *driver = nullptr;

    // Find trip
    for (int i = 0; i < tripCount; i++)
    {
        if (trips[i] && trips[i]->getTripId() == snap->tripId)
        {
            trip = trips[i];
            break;
        }
    }

    if (!trip)
        return false;

    driver = engine->getDriver(snap->driverId);

    // Restore based on operation type
    if (snap->operationType == 0) // ASSIGN
    {
        trip->setState(snap->previousState);
        if (driver)
        {
            driver->setAvailable(snap->driverWasAvailable);
            driver->setAssignedTripId(-1);
            // Restore driver location
            if (snap->driverLocation[0] != '\0')
            {
                driver->setCurrentNodeId(snap->driverLocation);
            }
        }
    }
    else if (snap->operationType == 1) // CANCEL
    {
        trip->setState(snap->previousState);
        if (driver && snap->driverId != -1)
        {
            driver->setAvailable(snap->driverWasAvailable);
            if (!snap->driverWasAvailable)
                driver->setAssignedTripId(snap->tripId);
            // Restore driver location
            if (snap->driverLocation[0] != '\0')
            {
                driver->setCurrentNodeId(snap->driverLocation);
            }
        }
    }
    else if (snap->operationType == 2) // COMPLETE
    {
        trip->setState(snap->previousState);
        if (driver)
        {
            driver->setAvailable(snap->driverWasAvailable);
            if (!snap->driverWasAvailable)
                driver->setAssignedTripId(snap->tripId);
            // Restore driver location
            if (snap->driverLocation[0] != '\0')
            {
                driver->setCurrentNodeId(snap->driverLocation);
            }
        }
    }
    else if (snap->operationType == 3) // RIDER_LOCATION_CHANGE
    {
        // Rider location will be restored by MainWindow
        // The snapshot contains riderLocation which MainWindow will use
    }
    else if (snap->operationType == 4) // DRIVER_AVAILABILITY_CHANGE
    {
        if (driver)
        {
            driver->setAvailable(snap->driverWasAvailable);
            // Restore driver location if available
            if (snap->driverLocation[0] != '\0')
            {
                driver->setCurrentNodeId(snap->driverLocation);
            }
        }
    }
    else if (snap->operationType == 11) // MOVEMENT
    {
        if (driver && snap->driverLocation[0] != '\0')
        {
            driver->setCurrentNodeId(snap->driverLocation);
        }
    }

    // Pop snapshot
    snapshotStack = snap->next;
    delete snap;
    operationCount--;

    return true;
}

bool RollbackManager::rollbackLastK(int k, Trip **trips, int tripCount,
                                   DispatchEngine *engine)
{
    for (int i = 0; i < k; i++)
    {
        if (!rollbackLast(trips, tripCount, engine))
            return false;
    }
    return true;
}

void RollbackManager::clearHistory()
{
    OperationSnapshot *current = snapshotStack;
    while (current != nullptr)
    {
        OperationSnapshot *temp = current;
        current = current->next;
        delete temp;
    }
    snapshotStack = nullptr;
    operationCount = 0;
}

int RollbackManager::getOperationCount() const
{
    return operationCount;
}

bool RollbackManager::canRollback() const
{
    return snapshotStack != nullptr;
}

void RollbackManager::displayHistory() const
{
    std::cout << "\n=== OPERATION HISTORY (" << operationCount << ") ===" << std::endl;
    OperationSnapshot *current = snapshotStack;
    int idx = 1;
    while (current != nullptr)
    {
        const char *opName = "UNKNOWN";
        if (current->operationType == 0)
            opName = "ASSIGN";
        else if (current->operationType == 1)
            opName = "CANCEL";
        else if (current->operationType == 2)
            opName = "COMPLETE";
        else if (current->operationType == 3)
            opName = "RIDER_LOCATION";
        else if (current->operationType == 4)
            opName = "DRIVER_AVAIL";
        else if (current->operationType == 10)
            opName = "DRIVER_ADD";
        else if (current->operationType == 11)
            opName = "MOVEMENT";
        else if (current->operationType == 20)
            opName = "TRIP_HISTORY";

        std::cout << idx << ". Trip #" << current->tripId << " | Op: " << opName
                  << " | Driver: " << current->driverId;
        if (current->operationType == 20)
        {
            std::cout << " | Status: " << current->details
                      << " | Fare: " << current->fare
                      << " | Dist: " << current->distance;
        }
        std::cout << std::endl;
        current = current->next;
        idx++;
    }
}
