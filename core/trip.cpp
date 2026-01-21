#include "trip.h"
#include <iostream>
#include <cstring>

Trip::Trip(int id, int rider, const char *pickup, const char *dropoff)
    : tripId(id), riderId(rider), driverId(-1), state(REQUESTED), currentPathIndex(0)
{
    strncpy(pickupNodeId, pickup, MAX_STRING_LENGTH - 1);
    pickupNodeId[MAX_STRING_LENGTH - 1] = '\0';
    strncpy(dropoffNodeId, dropoff, MAX_STRING_LENGTH - 1);
    dropoffNodeId[MAX_STRING_LENGTH - 1] = '\0';
    effectivePickupNodeId[0] = '\0';
    driverCurrentNodeId[0] = '\0';
    riderCurrentNodeId[0] = '\0';
}

Trip::~Trip()
{
}

int Trip::getTripId() const
{
    return tripId;
}

int Trip::getRiderId() const
{
    return riderId;
}

int Trip::getDriverId() const
{
    return driverId;
}

TripState Trip::getState() const
{
    return state;
}

const char *Trip::getPickupNodeId() const
{
    return pickupNodeId;
}

const char *Trip::getDropoffNodeId() const
{
    return dropoffNodeId;
}

const char *Trip::getEffectivePickupNodeId() const
{
    return effectivePickupNodeId;
}

const char *Trip::getDriverCurrentNodeId() const
{
    return driverCurrentNodeId;
}

const char *Trip::getRiderCurrentNodeId() const
{
    return riderCurrentNodeId;
}

const PathResult &Trip::getDriverToPickupPath() const
{
    return driverToPickupPath;
}

const PathResult &Trip::getPickupToDropoffPath() const
{
    return pickupToDropoffPath;
}

double Trip::getTotalDistance() const
{
    double total = 0.0;
    if (driverToPickupPath.totalDistance > 0)
        total += driverToPickupPath.totalDistance;
    if (pickupToDropoffPath.totalDistance > 0)
        total += pickupToDropoffPath.totalDistance;
    return total;
}

int Trip::getCurrentPathIndex() const
{
    return currentPathIndex;
}

bool Trip::isValidTransition(TripState from, TripState to)
{
    // REQUESTED -> ASSIGNED, CANCELLED
    if (from == REQUESTED && (to == ASSIGNED || to == CANCELLED))
        return true;
    // ASSIGNED -> PICKUP_IN_PROGRESS, CANCELLED
    if (from == ASSIGNED && (to == PICKUP_IN_PROGRESS || to == CANCELLED))
        return true;
    // PICKUP_IN_PROGRESS -> ONGOING, CANCELLED
    if (from == PICKUP_IN_PROGRESS && (to == ONGOING || to == CANCELLED))
        return true;
    // ONGOING -> COMPLETED
    if (from == ONGOING && to == COMPLETED)
        return true;
    return false;
}

bool Trip::transitionToAssigned(int driver)
{
    if (!isValidTransition(state, ASSIGNED))
        return false;
    driverId = driver;
    state = ASSIGNED;
    return true;
}

bool Trip::transitionToPickupInProgress()
{
    if (!isValidTransition(state, PICKUP_IN_PROGRESS))
        return false;
    state = PICKUP_IN_PROGRESS;
    currentPathIndex = 0;
    return true;
}

bool Trip::transitionToOngoing()
{
    if (!isValidTransition(state, ONGOING))
        return false;
    state = ONGOING;
    return true;
}

bool Trip::transitionToCompleted()
{
    if (!isValidTransition(state, COMPLETED))
        return false;
    state = COMPLETED;
    return true;
}

bool Trip::transitionToCancelled()
{
    if (!isValidTransition(state, CANCELLED))
        return false;
    state = CANCELLED;
    return true;
}

void Trip::setDriverToPickupPath(const PathResult &path)
{
    driverToPickupPath = path;
}

void Trip::setPickupToDropoffPath(const PathResult &path)
{
    pickupToDropoffPath = path;
}

void Trip::setState(TripState s)
{
    state = s;
}

void Trip::setEffectivePickupNodeId(const char *nodeId)
{
    strncpy(effectivePickupNodeId, nodeId, MAX_STRING_LENGTH - 1);
    effectivePickupNodeId[MAX_STRING_LENGTH - 1] = '\0';
}

void Trip::setDriverCurrentNodeId(const char *nodeId)
{
    strncpy(driverCurrentNodeId, nodeId, MAX_STRING_LENGTH - 1);
    driverCurrentNodeId[MAX_STRING_LENGTH - 1] = '\0';
}

void Trip::setRiderCurrentNodeId(const char *nodeId)
{
    strncpy(riderCurrentNodeId, nodeId, MAX_STRING_LENGTH - 1);
    riderCurrentNodeId[MAX_STRING_LENGTH - 1] = '\0';
}

void Trip::setCurrentPathIndex(int index)
{
    currentPathIndex = index;
}

bool Trip::advanceMovement()
{
    // Returns true if there are more nodes to traverse
    // This method should be called after locations are updated externally
    currentPathIndex++;
    
    // Check if we've reached the end of current path segment
    if (state == PICKUP_IN_PROGRESS)
    {
        return currentPathIndex < driverToPickupPath.pathLength;
    }
    else if (state == ONGOING)
    {
        return currentPathIndex < pickupToDropoffPath.pathLength;
    }
    
    return false;
}

const char *Trip::stateToString(TripState s) const
{
    switch (s)
    {
    case REQUESTED:
        return "REQUESTED";
    case ASSIGNED:
        return "ASSIGNED";
    case PICKUP_IN_PROGRESS:
        return "PICKUP_IN_PROGRESS";
    case ONGOING:
        return "ONGOING";
    case COMPLETED:
        return "COMPLETED";
    case CANCELLED:
        return "CANCELLED";
    default:
        return "UNKNOWN";
    }
}

void Trip::display() const
{
    std::cout << "Trip #" << tripId << " | Rider: " << riderId
              << " | Driver: " << (driverId == -1 ? -1 : driverId)
              << " | State: " << stateToString(state)
              << " | Distance: " << getTotalDistance() << "m" << std::endl;
}

// Extract zone name from node ID
// Example: "zone4_township-B7_S6_Loc9" -> "zone4"
void Trip::extractZone(const char *nodeId, char *zone, int maxLen)
{
    if (!nodeId || !zone || maxLen <= 0)
        return;
    
    const char *start = nodeId;
    int len = 0;
    
    // Find the underscore after zone number
    while (nodeId[len] && nodeId[len] != '_' && len < maxLen - 1)
    {
        len++;
    }
    
    // Copy zone part (e.g., "zone4")
    strncpy(zone, start, len);
    zone[len] = '\0';
}

// Calculate base fare: 150 rupees per 1000 meters
// Total distance = driver to pickup + pickup to dropoff
double Trip::calculateBaseFare() const
{
    double totalDistance = getTotalDistance();
    // Rate: 150 rupees per 1000 meters = 0.15 rupees per meter
    double baseFare = (totalDistance / 1000.0) * 150.0;
    return baseFare;
}

// Calculate zone surcharge: 100 rupees if pickup and dropoff in different zones
double Trip::calculateZoneSurcharge() const
{
    char pickupZone[MAX_STRING_LENGTH] = {0};
    char dropoffZone[MAX_STRING_LENGTH] = {0};
    
    extractZone(pickupNodeId, pickupZone, MAX_STRING_LENGTH);
    extractZone(dropoffNodeId, dropoffZone, MAX_STRING_LENGTH);
    
    // If zones are different, add 100 rupees surcharge
    if (strcmp(pickupZone, dropoffZone) != 0)
    {
        return 100.0;
    }
    return 0.0;
}

// Calculate total fare including base fare and surcharge
double Trip::calculateTotalFare() const
{
    return calculateBaseFare() + calculateZoneSurcharge();
}
