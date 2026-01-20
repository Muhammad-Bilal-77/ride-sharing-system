#include "ridesharesystem.h"
#include <iostream>
#include <cstring>

RideShareSystem::RideShareSystem(City *c)
    : city(c), nextTripId(1), nextRiderId(1)
{
    dispatchEngine = new DispatchEngine(c, 100, 200);
    rollbackManager = new RollbackManager(200);
}

RideShareSystem::~RideShareSystem()
{
    delete dispatchEngine;
    delete rollbackManager;
}

bool RideShareSystem::addDriver(int driverId, const char *nodeId, const char *zone)
{
    return dispatchEngine->addDriver(driverId, nodeId, zone);
}

bool RideShareSystem::createAndRequestTrip(int riderId, const char *pickupNodeId,
                                          const char *dropoffNodeId)
{
    if (!dispatchEngine->requestTrip(nextTripId, riderId, pickupNodeId, dropoffNodeId))
        return false;
    
    std::cout << "[TRIP] Created Trip #" << nextTripId << " for Rider #" << riderId << std::endl;
    nextTripId++;
    return true;
}

bool RideShareSystem::assignTrip(int tripId, int driverId)
{
    Trip *trip = getTrip(tripId);
    Driver *driver = getDriver(driverId);
    
    if (!trip || !driver)
        return false;

    // Record snapshot before assignment
    rollbackManager->recordSnapshot(0, tripId, driverId, trip->getState(), driver->isAvailable());

    if (!dispatchEngine->assignTrip(tripId, driverId))
        return false;

    std::cout << "[ASSIGN] Trip #" << tripId << " -> Driver #" << driverId << std::endl;
    return true;
}

bool RideShareSystem::startTrip(int tripId)
{
    Trip *trip = getTrip(tripId);
    if (!trip || !dispatchEngine->startTrip(tripId))
        return false;

    std::cout << "[START] Trip #" << tripId << " is now ONGOING" << std::endl;
    return true;
}

bool RideShareSystem::completeTrip(int tripId)
{
    Trip *trip = getTrip(tripId);
    Driver *driver = trip ? getDriver(trip->getDriverId()) : nullptr;
    
    if (!trip || !driver)
        return false;

    // Record snapshot before completion
    rollbackManager->recordSnapshot(2, tripId, trip->getDriverId(), 
                                   trip->getState(), driver->isAvailable());

    if (!dispatchEngine->completeTrip(tripId))
        return false;

    std::cout << "[COMPLETE] Trip #" << tripId << " completed. Distance: " 
              << trip->getTotalDistance() << "m" << std::endl;
    return true;
}

bool RideShareSystem::cancelTrip(int tripId)
{
    Trip *trip = getTrip(tripId);
    Driver *driver = trip && trip->getDriverId() != -1 ? getDriver(trip->getDriverId()) : nullptr;
    
    if (!trip)
        return false;

    // Record snapshot before cancellation
    rollbackManager->recordSnapshot(1, tripId, trip->getDriverId(), 
                                   trip->getState(), driver ? driver->isAvailable() : true);

    if (!dispatchEngine->cancelTrip(tripId))
        return false;

    std::cout << "[CANCEL] Trip #" << tripId << " cancelled" << std::endl;
    return true;
}

bool RideShareSystem::rollbackLastOperation()
{
    if (!rollbackManager->canRollback())
        return false;

    Trip **trips = new Trip *[dispatchEngine->getTripCount()];
    for (int i = 0; i < dispatchEngine->getTripCount(); i++)
        trips[i] = dispatchEngine->getTrip(i + 1);

    bool result = rollbackManager->rollbackLast(trips, dispatchEngine->getTripCount(),
                                               dispatchEngine);
    delete[] trips;

    if (result)
        std::cout << "[ROLLBACK] Last operation rolled back" << std::endl;

    return result;
}

bool RideShareSystem::rollbackLastKOperations(int k)
{
    if (!rollbackManager->canRollback())
        return false;

    Trip **trips = new Trip *[dispatchEngine->getTripCount()];
    for (int i = 0; i < dispatchEngine->getTripCount(); i++)
        trips[i] = dispatchEngine->getTrip(i + 1);

    bool result = rollbackManager->rollbackLastK(k, trips, dispatchEngine->getTripCount(),
                                                 dispatchEngine);
    delete[] trips;

    if (result)
        std::cout << "[ROLLBACK] Last " << k << " operations rolled back" << std::endl;

    return result;
}

AnalyticsData RideShareSystem::getAnalytics() const
{
    AnalyticsData data;
    data.driverCount = dispatchEngine->getTripCount();

    for (int i = 1; i <= dispatchEngine->getTripCount(); i++)
    {
        Trip *trip = dispatchEngine->getTrip(i);
        if (trip)
        {
            data.totalTrips++;
            if (trip->getState() == COMPLETED)
                data.completedTrips++;
            else if (trip->getState() == CANCELLED)
                data.cancelledTrips++;
            data.totalDistance += trip->getTotalDistance();
        }
    }

    return data;
}

double RideShareSystem::getAverageTripDistance() const
{
    AnalyticsData data = getAnalytics();
    if (data.completedTrips == 0)
        return 0.0;
    return data.totalDistance / data.completedTrips;
}

double RideShareSystem::getDriverUtilizationPercentage() const
{
    // Percentage of drivers with assigned trips
    int activeCount = dispatchEngine->getActiveTripsCount();
    int totalDrivers = 0;

    for (int i = 1; i <= 100; i++)
    {
        if (getDriver(i))
            totalDrivers++;
        else
            break;
    }

    if (totalDrivers == 0)
        return 0.0;
    return (activeCount * 100.0) / totalDrivers;
}

Trip *RideShareSystem::getTrip(int tripId) const
{
    return dispatchEngine->getTrip(tripId);
}

Driver *RideShareSystem::getDriver(int driverId) const
{
    return dispatchEngine->getDriver(driverId);
}

void RideShareSystem::displaySystem() const
{
    std::cout << "\n========================================" << std::endl;
    std::cout << "    RIDE SHARE SYSTEM STATUS" << std::endl;
    std::cout << "========================================" << std::endl;

    dispatchEngine->displayDrivers();
    dispatchEngine->displayTrips();
    dispatchEngine->displayActiveTrips();
    rollbackManager->displayHistory();
}

void RideShareSystem::displayAnalytics() const
{
    AnalyticsData data = getAnalytics();

    std::cout << "\n========================================" << std::endl;
    std::cout << "    ANALYTICS REPORT" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Total Trips: " << data.totalTrips << std::endl;
    std::cout << "Completed: " << data.completedTrips << std::endl;
    std::cout << "Cancelled: " << data.cancelledTrips << std::endl;
    std::cout << "Average Distance: " << getAverageTripDistance() << " m" << std::endl;
    std::cout << "Driver Utilization: " << getDriverUtilizationPercentage() << "%" << std::endl;
}
