#include "dispatchengine.h"
#include <iostream>
#include <cstring>
#include <cmath>

// Constructor
DispatchEngine::DispatchEngine(City *c, int maxD, int maxT)
    : city(c), driverCount(0), maxDrivers(maxD), tripCount(0), 
      maxTrips(maxT), activeTripsHead(nullptr)
{
    drivers = new Driver *[maxDrivers];
    trips = new Trip *[maxTrips];
    rollbackManager = new RollbackManager(500);  // Support up to 500 operations
    
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
    
    // Clean up rollback manager
    delete rollbackManager;
}

bool DispatchEngine::addDriver(int driverId, const char *nodeId, const char *zone)
{
    if (driverCount >= maxDrivers)
        return false;
    
    // POLICY: Driver must be on a route node
    if (!validateDriverNode(nodeId))
    {
        std::cout << "[ERROR] Driver location must be a route node (street/highway). Node: " << nodeId << std::endl;
        return false;
    }
    
    // Record snapshot for rollback (driver creation)
    // Operation type can be extended; for now using custom type 10 for driver_add
    rollbackManager->recordSnapshot(10, -1, driverId, REQUESTED, true);
    
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
    
    // Record snapshot before assignment
    rollbackManager->recordSnapshot(0, tripId, driverId, trip->getState(), driver->isAvailable());
    
    if (!trip->transitionToAssigned(driverId))
        return false;
    
    // POLICY: Resolve rider pickup node (route node enforcement)
    const char *effectivePickupNode = resolveRiderPickupNode(trip->getPickupNodeId());
    trip->setEffectivePickupNodeId(effectivePickupNode);
    
    std::cout << "[PICKUP RESOLUTION] Rider at: " << trip->getPickupNodeId() 
              << " -> Effective pickup: " << effectivePickupNode << std::endl;
    
    // Compute path from driver to effective pickup
    PathResult driverPath = city->findShortestPathAStar(driver->getCurrentNodeId(),
                                                       effectivePickupNode);
    trip->setDriverToPickupPath(driverPath);
    
    // Compute path from effective pickup to dropoff
    PathResult riderPath = city->findShortestPathAStar(effectivePickupNode,
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
        // Record snapshot before completion
        rollbackManager->recordSnapshot(2, tripId, trip->getDriverId(), 
                                      trip->getState(), driver->isAvailable());
        
        // POLICY: Driver relocation after drop-off
        Node *dropNode = city->getNode(trip->getDropoffNodeId());
        if (dropNode)
        {
            if (strcmp(dropNode->locationType, "street") == 0 || 
                strcmp(dropNode->locationType, "highway") == 0)
            {
                // Drop location is a route node - driver stays there
                driver->setCurrentNodeId(trip->getDropoffNodeId());
                std::cout << "[COMPLETION] Driver remains at route node: " 
                         << trip->getDropoffNodeId() << std::endl;
            }
            else
            {
                // Drop location is not a route node - relocate to nearest route node
                const char *nearestRoute = findNearestRouteNode(dropNode->x, dropNode->y);
                if (nearestRoute)
                {
                    driver->setCurrentNodeId(nearestRoute);
                    std::cout << "[COMPLETION] Driver relocated from " << trip->getDropoffNodeId() 
                             << " to nearest route node: " << nearestRoute << std::endl;
                }
                else
                {
                    // Fallback: keep at drop location (shouldn't happen in valid graph)
                    driver->setCurrentNodeId(trip->getDropoffNodeId());
                }
            }
        }
        
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
        // Record snapshot before cancellation
        rollbackManager->recordSnapshot(1, tripId, trip->getDriverId(), 
                                      trip->getState(), driver->isAvailable());
        
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

// ============= NEW HELPER METHODS =============

// Validates if a node is a valid route node for driver placement
bool DispatchEngine::validateDriverNode(const char *nodeId) const
{
    Node *node = city->getNode(nodeId);
    if (!node)
        return false;
    
    // Driver can only be on route nodes: street or highway
    return (strcmp(node->locationType, "street") == 0 || 
            strcmp(node->locationType, "highway") == 0);
}

// Finds nearest route node to given coordinates
const char *DispatchEngine::findNearestRouteNode(double x, double y) const
{
    Node *current = city->getFirstNode();
    Node *bestNode = nullptr;
    double minDist = 1e9;
    
    while (current != nullptr)
    {
        // Only consider route nodes
        if (strcmp(current->locationType, "street") == 0 || 
            strcmp(current->locationType, "highway") == 0)
        {
            double dx = current->x - x;
            double dy = current->y - y;
            double dist = sqrt(dx * dx + dy * dy);
            
            if (dist < minDist)
            {
                minDist = dist;
                bestNode = current;
            }
        }
        current = current->next;
    }
    
    return bestNode ? bestNode->id : nullptr;
}

// Resolves rider pickup node based on policy
const char *DispatchEngine::resolveRiderPickupNode(const char *riderNodeId)
{
    Node *node = city->getNode(riderNodeId);
    if (!node)
        return riderNodeId;  // Return as-is if not found
    
    // If already a route node, use it directly
    if (strcmp(node->locationType, "street") == 0 || 
        strcmp(node->locationType, "highway") == 0)
    {
        return node->id;
    }
    
    // Otherwise, find nearest route node
    return findNearestRouteNode(node->x, node->y);
}

// Start pickup movement (driver to pickup location)
bool DispatchEngine::startPickupMovement(int tripId)
{
    Trip *trip = getTrip(tripId);
    if (!trip || !trip->transitionToPickupInProgress())
        return false;
    
    Driver *driver = getDriver(trip->getDriverId());
    if (!driver)
        return false;
    
    // Initialize current locations
    trip->setDriverCurrentNodeId(driver->getCurrentNodeId());
    trip->setRiderCurrentNodeId(trip->getPickupNodeId());
    trip->setCurrentPathIndex(0);
    
    std::cout << "[MOVEMENT] Started pickup movement for Trip #" << tripId << std::endl;
    return true;
}

// Advance trip movement by one step (1 second)
bool DispatchEngine::advanceTripMovement(int tripId)
{
    Trip *trip = getTrip(tripId);
    if (!trip)
        return false;
    
    Driver *driver = getDriver(trip->getDriverId());
    if (!driver)
        return false;
    
    int currentIndex = trip->getCurrentPathIndex();
    
    if (trip->getState() == PICKUP_IN_PROGRESS)
    {
        // Moving driver to pickup
        const PathResult &path = trip->getDriverToPickupPath();
        
        if (currentIndex >= path.pathLength - 1)
        {
            // Reached pickup location
            trip->setDriverCurrentNodeId(trip->getEffectivePickupNodeId());
            driver->setCurrentNodeId(trip->getEffectivePickupNodeId());
            
            // Record location change snapshot
            rollbackManager->recordSnapshot(11, tripId, driver->getDriverId(), 
                                          trip->getState(), false);
            
            // Transition to ONGOING
            trip->transitionToOngoing();
            trip->setCurrentPathIndex(0);
            
            std::cout << "[MOVEMENT] Driver reached pickup for Trip #" << tripId << std::endl;
            return true;
        }
        else
        {
            // Move to next node
            currentIndex++;
            trip->setCurrentPathIndex(currentIndex);
            trip->setDriverCurrentNodeId(path.path[currentIndex]);
            driver->setCurrentNodeId(path.path[currentIndex]);
            
            // Record movement snapshot
            rollbackManager->recordSnapshot(11, tripId, driver->getDriverId(), 
                                          trip->getState(), false);
            return true;
        }
    }
    else if (trip->getState() == ONGOING)
    {
        // Moving from pickup to dropoff (both driver and rider)
        const PathResult &path = trip->getPickupToDropoffPath();
        
        if (currentIndex >= path.pathLength - 1)
        {
            // Reached destination - will be handled by completeTrip
            return false;
        }
        else
        {
            // Move to next node
            currentIndex++;
            trip->setCurrentPathIndex(currentIndex);
            trip->setDriverCurrentNodeId(path.path[currentIndex]);
            trip->setRiderCurrentNodeId(path.path[currentIndex]);
            driver->setCurrentNodeId(path.path[currentIndex]);
            
            // Record movement snapshot
            rollbackManager->recordSnapshot(11, tripId, driver->getDriverId(), 
                                          trip->getState(), false);
            return true;
        }
    }
    
    return false;
}

// Get rollback manager
RollbackManager *DispatchEngine::getRollbackManager() const
{
    return rollbackManager;
}
