#ifndef RIDER_H
#define RIDER_H

#include "city.h"

class Rider
{
private:
    int riderId;
    char pickupNodeId[MAX_STRING_LENGTH];
    char dropoffNodeId[MAX_STRING_LENGTH];

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

    // Display
    void display() const;
};

#endif // RIDER_H
