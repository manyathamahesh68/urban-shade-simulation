
#pragma once

#include "solar_position.h"

class SolarController {
public:
    SolarInput input;
    SolarPosition position;

    bool animationEnabled = false;

    // Simulated seconds per real second.
    double simulationSpeed = 60.0;

    void update(double deltaSeconds);

    void recalculate();

    void setTime(int hour, int minute);

    void changeTime(int minutes);

    void setLocation(
        double latitude,
        double longitude,
        double utcOffset
    );

    void setDate(int year, int month, int day);

    void toggleAnimation();
};
