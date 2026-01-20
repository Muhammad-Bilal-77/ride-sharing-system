#include "dispatchengine.h"
#include <iostream>
#include <cstring>

// Constructor
DispatchEngine::DispatchEngine(City *c, int maxD, int maxT)
    : city(c), driverCount(0), maxDrivers(maxD), tripCount(0), 
      maxTrips(maxT), activeTripsHead(nullptr)
{
    drivers = new Driver *[maxDrivers];
    trips = new Trip *[maxTrips];
    
    for (int i = 0; i < maxDrivers; i++)
        drivers[i] = nullptr;
    for (int i = 0; i < maxTrips; i++)
        trips[i] = nullptr;
}

// Destructor
DispatchEngine::~DispatchEngine()
{
    // Clean up active trips list
    ActiveTrip *current = activeTripsHead;
    while (current != nullptr)
    {
        ActiveTrip *temp = current;
        current = current->next;
        delete temp;
    }
    
    // Clean up drivers
    for (int i = 0; i < driverCount; i++)
        delete drivers[i];
    delete[] drivers;
    
    // Clean up trips
    for (int i = 0; i < tripCount; i++)
        delete trips[i];
    delete[] trips;
}

bool DispatchEngine::addDriver(int driverId, const char *nodeId, const char *zone)
{
    if (driverCount >= maxDrivers)
        return false;
    
    drivers[driverCount] = new Driver(driverId, nodeId, zone);
    driverCount++;
    return true;
}

bool DispatchEngine::removeDriver(int driverId)
{
    for (int i = 0; i < driverCount; i++)
    {
        if (drivers[i] && drivers[i]->getDriverId() == driverId)
        {
            delete drivers[i];
            drivers[i] = drivers[driverCount - 1];
            drivers[driverCount - 1] = nullptr;
            driverCount--;
            return true;
        }
    }
    return false;
}

Driver *DispatchEngine::getDriver(int driverId) const
{
    for (int i = 0; i < driverCount; i++)
    {
        if (drivers[i] && drivers[i]->getDriverId() == driverId)
            return drivers[i];
    }
    return nullptr;
}

bool DispatchEngine::requestTrip(int tripId, int riderId, const char *pickupNodeId,
                                const char *dropoffNodeId)
{
    if (tripCount >= maxTrips)
        return false;
    
    trips[tripCount] = new Trip(tripId, riderId, pickupNodeId, dropoffNodeId);
    tripCount++;
    return true;
}

int DispatchEngine::findNearestAvailableDriver(const char *pickupNodeId, bool sameZone)
{
    Driver *best = nullptr;
    double minDist = 1e9;
    
    for (int i = 0; i < driverCount; i++)
    {
        if (!drivers[i] || !drivers[i]->isAvailable())
            continue;
        
        double dist = city->getDistance(drivers[i]->getCurrentNodeId(), pickupNodeId);
        if (dist < 0)
            continue;
        
        if (best == nullptr || dist < minDist)
        {
            best = drivers[i];
            minDist = dist;
        }
    }
    
    return best ? best->getDriverId() : -1;
}

Driver *DispatchEngine::selectBestDriver(Driver **candidates, int count,
                                        const char *pickupNodeId, bool sameZonePref)
{
    if (count == 0)
        return nullptr;
    
    Driver *best = candidates[0];
    double minDist = city->getDistance(best->getCurrentNodeId(), pickupNodeId);
    
    for (int i = 1; i < count; i++)
    {
        double dist = city->getDistance(candidates[i]->getCurrentNodeId(), pickupNodeId);
        if (dist < minDist)
        {
            best = candidates[i];
            minDist = dist;
        }
    }
    
    return best;
}

bool DispatchEngine::assignTrip(int tripId, int driverId)
{
    Trip *trip = getTrip(tripId);
    Driver *driver = getDriver(driverId);
    
    if (!trip || !driver || !driver->isAvailable())
        return false;
    
    if (!trip->transitionToAssigned(driverId))
        return false;
    
    // Compute path from driver to pickup
    PathResult driverPath = city->findShortestPathAStar(driver->getCurrentNodeId(),
                                                       trip->getPickupNodeId());
    trip->setDriverToPickupPath(driverPath);
    
    // Compute path from pickup to dropoff
    PathResult riderPath = city->findShortestPathAStar(trip->getPickupNodeId(),
                                                      trip->getDropoffNodeId());
    trip->setPickupToDropoffPath(riderPath);
    
    driver->setAvailable(false);
    driver->setAssignedTripId(tripId);
    addActiveTrip(trip, driver);
    
    return true;
}

bool DispatchEngine::startTrip(int tripId)
{
    Trip *trip = getTrip(tripId);
    if (!trip || !trip->transitionToOngoing())
        return false;
    
    return true;
}

bool DispatchEngine::completeTrip(int tripId)
{
    Trip *trip = getTrip(tripId);
    if (!trip || !trip->transitionToCompleted())
        return false;
    
    Driver *driver = getDriver(trip->getDriverId());
    if (driver)
    {
        driver->setAvailable(true);
        driver->setAssignedTripId(-1);
    }
    
    removeActiveTrip(tripId);
    return true;
}

bool DispatchEngine::cancelTrip(int tripId)
{
    Trip *trip = getTrip(tripId);
    if (!trip || !trip->transitionToCancelled())
        return false;
    
    Driver *driver = getDriver(trip->getDriverId());
    if (driver && driver->getAssignedTripId() == tripId)
    {
        driver->setAvailable(true);
        driver->setAssignedTripId(-1);
    }
    
    removeActiveTrip(tripId);
    return true;
}

int DispatchEngine::getTripCount() const
{
    return tripCount;
}

Trip *DispatchEngine::getTrip(int tripId) const
{
    for (int i = 0; i < tripCount; i++)
    {
        if (trips[i] && trips[i]->getTripId() == tripId)
            return trips[i];
    }
    return nullptr;
}

void DispatchEngine::addActiveTrip(Trip *trip, Driver *driver)
{
    ActiveTrip *newTrip = new ActiveTrip(trip, driver);
    newTrip->next = activeTripsHead;
    activeTripsHead = newTrip;
}

void DispatchEngine::removeActiveTrip(int tripId)
{
    ActiveTrip *current = activeTripsHead;
    ActiveTrip *prev = nullptr;
    
    while (current != nullptr)
    {
        if (current->trip && current->trip->getTripId() == tripId)
        {
            if (prev)
                prev->next = current->next;
            else
                activeTripsHead = current->next;
            delete current;
            return;
        }
        prev = current;
        current = current->next;
    }
}

ActiveTrip *DispatchEngine::findActiveTrip(int tripId)
{
    ActiveTrip *current = activeTripsHead;
    while (current != nullptr)
    {
        if (current->trip && current->trip->getTripId() == tripId)
            return current;
        current = current->next;
    }
    return nullptr;
}

ActiveTrip *DispatchEngine::getActiveTripsHead() const
{
    return activeTripsHead;
}

int DispatchEngine::getActiveTripsCount() const
{
    int count = 0;
    ActiveTrip *current = activeTripsHead;
    while (current != nullptr)
    {
        count++;
        current = current->next;
    }
    return count;
}

void DispatchEngine::displayDrivers() const
{
    std::cout << "\n=== DRIVERS (" << driverCount << ") ===" << std::endl;
    for (int i = 0; i < driverCount; i++)
    {
        if (drivers[i])
            drivers[i]->display();
    }
}

void DispatchEngine::displayTrips() const
{
    std::cout << "\n=== TRIPS (" << tripCount << ") ===" << std::endl;
    for (int i = 0; i < tripCount; i++)
    {
        if (trips[i])
            trips[i]->display();
    }
}

void DispatchEngine::displayActiveTrips() const
{
    std::cout << "\n=== ACTIVE TRIPS (" << getActiveTripsCount() << ") ===" << std::endl;
    ActiveTrip *current = activeTripsHead;
    while (current != nullptr)
    {
        if (current->trip)
            current->trip->display();
        current = current->next;
    }
}
