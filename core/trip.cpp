#include "trip.h"
#include <iostream>
#include <cstring>

Trip::Trip(int id, int rider, const char *pickup, const char *dropoff)
    : tripId(id), riderId(rider), driverId(-1), state(REQUESTED)
{
    strncpy(pickupNodeId, pickup, MAX_STRING_LENGTH - 1);
    pickupNodeId[MAX_STRING_LENGTH - 1] = '\0';
    strncpy(dropoffNodeId, dropoff, MAX_STRING_LENGTH - 1);
    dropoffNodeId[MAX_STRING_LENGTH - 1] = '\0';
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

bool Trip::isValidTransition(TripState from, TripState to)
{
    // REQUESTED -> ASSIGNED, CANCELLED
    if (from == REQUESTED && (to == ASSIGNED || to == CANCELLED))
        return true;
    // ASSIGNED -> ONGOING, CANCELLED
    if (from == ASSIGNED && (to == ONGOING || to == CANCELLED))
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

const char *Trip::stateToString(TripState s) const
{
    switch (s)
    {
    case REQUESTED:
        return "REQUESTED";
    case ASSIGNED:
        return "ASSIGNED";
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
