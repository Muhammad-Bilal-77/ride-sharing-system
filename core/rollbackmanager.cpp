#include "rollbackmanager.h"
#include "dispatchengine.h"
#include <iostream>

RollbackManager::RollbackManager(int maxOps)
    : snapshotStack(nullptr), operationCount(0), maxOperations(maxOps)
{
}

RollbackManager::~RollbackManager()
{
    clearHistory();
}

void RollbackManager::recordSnapshot(int opType, int tripId, int driverId,
                                    TripState state, bool driverAvail)
{
    if (operationCount >= maxOperations)
        return;

    OperationSnapshot *snap = new OperationSnapshot();
    snap->operationType = opType;
    snap->tripId = tripId;
    snap->driverId = driverId;
    snap->previousState = state;
    snap->driverWasAvailable = driverAvail;
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

        std::cout << idx << ". Trip #" << current->tripId << " | Op: " << opName
                  << " | Driver: " << current->driverId << std::endl;
        current = current->next;
        idx++;
    }
}
