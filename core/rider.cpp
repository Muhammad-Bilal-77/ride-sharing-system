#include "rider.h"
#include <iostream>
#include <cstring>

Rider::Rider(int id, const char *pickup, const char *dropoff)
    : riderId(id)
{
    strncpy(pickupNodeId, pickup, MAX_STRING_LENGTH - 1);
    pickupNodeId[MAX_STRING_LENGTH - 1] = '\0';
    strncpy(dropoffNodeId, dropoff, MAX_STRING_LENGTH - 1);
    dropoffNodeId[MAX_STRING_LENGTH - 1] = '\0';
}

Rider::~Rider()
{
}

int Rider::getRiderId() const
{
    return riderId;
}

const char *Rider::getPickupNodeId() const
{
    return pickupNodeId;
}

const char *Rider::getDropoffNodeId() const
{
    return dropoffNodeId;
}

void Rider::setPickupNodeId(const char *nodeId)
{
    strncpy(pickupNodeId, nodeId, MAX_STRING_LENGTH - 1);
    pickupNodeId[MAX_STRING_LENGTH - 1] = '\0';
}

void Rider::setDropoffNodeId(const char *nodeId)
{
    strncpy(dropoffNodeId, nodeId, MAX_STRING_LENGTH - 1);
    dropoffNodeId[MAX_STRING_LENGTH - 1] = '\0';
}

void Rider::display() const
{
    std::cout << "Rider #" << riderId << " | Pickup: " << pickupNodeId
              << " | Dropoff: " << dropoffNodeId << std::endl;
}
