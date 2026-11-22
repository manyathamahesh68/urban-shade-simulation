
#include "solar_controller.h"

#include <cmath>
#include <stdexcept>

void SolarController::recalculate() {
    position = SolarCalculator::calculate(input);
}

void SolarController::setTime(
    int hour, int minute
) {
    if (hour < 0 || hour > 23 ||
        minute < 0 || minute > 59) {
        throw std::invalid_argument("Invalid time");
    }

    input.hour = hour;
    input.minute = minute;
    input.second = 0;

    recalculate();
}

void SolarController::changeTime(int minutes) {
    int total =
        input.hour * 60 +
        input.minute +
        minutes;

    total = (total % 1440 + 1440) % 1440;

    input.hour = total / 60;
    input.minute = total % 60;
    input.second = 0;

    recalculate();
}

void SolarController::setLocation(
    double latitude,
    double longitude,
    double utcOffset
) {
    input.latitude = latitude;
    input.longitude = longitude;
    input.utcOffsetHours = utcOffset;

    recalculate();
}

void SolarController::setDate(
    int year, int month, int day
) {
    input.year = year;
    input.month = month;
    input.day = day;

    recalculate();
}

void SolarController::toggleAnimation() {
    animationEnabled = !animationEnabled;
}

void SolarController::update(double deltaSeconds) {
    if (!animationEnabled || deltaSeconds <= 0.0)
        return;

    double currentMinutes =
        input.hour * 60.0 +
        input.minute +
        input.second / 60.0;

    double elapsedMinutes =
        deltaSeconds * simulationSpeed / 60.0;

    double total = std::fmod(
        currentMinutes + elapsedMinutes,
        1440.0
    );

    if (total < 0.0)
        total += 1440.0;

    input.hour = static_cast<int>(total / 60.0);

    double remaining =
        total - input.hour * 60.0;

    input.minute = static_cast<int>(remaining);

    input.second = static_cast<int>(
        (remaining - input.minute) * 60.0
    );

    recalculate();
}
