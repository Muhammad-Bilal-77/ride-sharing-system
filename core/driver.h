#ifndef DRIVER_H
#define DRIVER_H

#include "city.h"

class Driver
{
private:
    int driverId;
    char currentNodeId[MAX_STRING_LENGTH];
    char zone[MAX_STRING_LENGTH];
    bool available;
    int assignedTripId; // -1 if no trip assigned

public:
    Driver(int id, const char *nodeId, const char *driverZone);
    ~Driver();

    // Getters
    int getDriverId() const;
    const char *getCurrentNodeId() const;
    const char *getZone() const;
    bool isAvailable() const;
    int getAssignedTripId() const;

    // Setters
    void setCurrentNodeId(const char *nodeId);
    void setZone(const char *driverZone);
    void setAvailable(bool avail);
    void setAssignedTripId(int tripId);

    // Display
    void display() const;
};

#endif // DRIVER_H
