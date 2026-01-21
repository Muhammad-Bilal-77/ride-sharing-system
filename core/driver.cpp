#include "driver.h"
#include <iostream>
#include <cstring>

Driver::Driver(int id, const char *nodeId, const char *driverZone)
    : driverId(id), available(true), assignedTripId(-1)
{
    strncpy(currentNodeId, nodeId, MAX_STRING_LENGTH - 1);
    currentNodeId[MAX_STRING_LENGTH - 1] = '\0';
    strncpy(zone, driverZone, MAX_STRING_LENGTH - 1);
    zone[MAX_STRING_LENGTH - 1] = '\0';
}

Driver::~Driver()
{
}

int Driver::getDriverId() const
{
    return driverId;
}

const char *Driver::getCurrentNodeId() const
{
    return currentNodeId;
}

const char *Driver::getZone() const
{
    return zone;
}

bool Driver::isAvailable() const
{
    return available;
}

int Driver::getAssignedTripId() const
{
    return assignedTripId;
}

void Driver::setCurrentNodeId(const char *nodeId)
{
    strncpy(currentNodeId, nodeId, MAX_STRING_LENGTH - 1);
    currentNodeId[MAX_STRING_LENGTH - 1] = '\0';
}

void Driver::setZone(const char *driverZone)
{
    strncpy(zone, driverZone, MAX_STRING_LENGTH - 1);
    zone[MAX_STRING_LENGTH - 1] = '\0';
}

void Driver::setAvailable(bool avail)
{
    available = avail;
}

void Driver::setAssignedTripId(int tripId)
{
    assignedTripId = tripId;
}

void Driver::display() const
{
    std::cout << "Driver #" << driverId << " | Node: " << currentNodeId
              << " | Zone: " << zone << " | Available: " << (available ? "YES" : "NO")
              << " | Trip: " << (assignedTripId == -1 ? "NONE" : std::to_string(assignedTripId)) << std::endl;
}
