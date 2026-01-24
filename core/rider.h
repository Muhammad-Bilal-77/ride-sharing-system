#ifndef RIDER_H
#define RIDER_H

#include "city.h"
#include <vector>
#include <string>

struct TripHistoryRecord
{
    int tripId;
    std::string pickupNode;
    std::string dropoffNode;
    std::string status;  // "COMPLETED" or "CANCELLED"
    double fare;
    double distance;
    int driverId;
    std::string timestamp;
};

class Rider
{
private:
    int riderId;
    char pickupNodeId[MAX_STRING_LENGTH];
    char dropoffNodeId[MAX_STRING_LENGTH];
    std::vector<TripHistoryRecord> tripHistory;

public:
    Rider(int id, const char *pickup, const char *dropoff);
    ~Rider();

    // Getters
    int getRiderId() const;
    const char *getPickupNodeId() const;
    const char *getDropoffNodeId() const;

    // Setters
    void setPickupNodeId(const char *nodeId);
    void setDropoffNodeId(const char *nodeId);

    // Trip History
    void addTripToHistory(int tripId, const char *pickup, const char *dropoff, 
                          const char *status, double fare, double distance, int driverId);
    const std::vector<TripHistoryRecord>& getTripHistory() const;
    int getTripHistoryCount() const;

    // Display
    void display() const;
};

#endif // RIDER_H
