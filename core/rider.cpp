#include "rider.h"
#include <iostream>
#include <cstring>
#include <ctime>
#include <sstream>
#include <iomanip>

static std::string getCurrentTimestamp()
{
    time_t now = time(nullptr);
    struct tm *timeinfo = localtime(&now);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return std::string(buffer);
}

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

void Rider::addTripToHistory(int tripId, const char *pickup, const char *dropoff,
                              const char *status, double fare, double distance, int driverId)
{
    TripHistoryRecord record;
    record.tripId = tripId;
    record.pickupNode = pickup ? pickup : "";
    record.dropoffNode = dropoff ? dropoff : "";
    record.status = status ? status : "UNKNOWN";
    record.fare = fare;
    record.distance = distance;
    record.driverId = driverId;
    record.timestamp = getCurrentTimestamp();
    tripHistory.push_back(record);
}

const std::vector<TripHistoryRecord>& Rider::getTripHistory() const
{
    return tripHistory;
}

int Rider::getTripHistoryCount() const
{
    return static_cast<int>(tripHistory.size());
}
