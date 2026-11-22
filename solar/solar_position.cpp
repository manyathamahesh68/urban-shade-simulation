
#include "solar_position.h"

#include <cmath>
#include <stdexcept>
#include <algorithm>

namespace {
    constexpr double PI = 3.14159265358979323846;

    double toRadians(double degrees) {
        return degrees * PI / 180.0;
    }

    double toDegrees(double radians) {
        return radians * 180.0 / PI;
    }

    bool isLeapYear(int year) {
        return (year % 4 == 0 && year % 100 != 0)
               || year % 400 == 0;
    }

    int daysInMonth(int year, int month) {
        const int days[] = {
            31, 28, 31, 30, 31, 30,
            31, 31, 30, 31, 30, 31
        };

        if (month == 2 && isLeapYear(year))
            return 29;

        return days[month - 1];
    }

    void adjustDate(
        int& year, int& month, int& day,
        int offset
    ) {
        day += offset;

        if (day < 1) {
            month--;

            if (month < 1) {
                month = 12;
                year--;
            }

            day = daysInMonth(year, month);
        }
        else if (day > daysInMonth(year, month)) {
            day = 1;
            month++;

            if (month > 12) {
                month = 1;
                year++;
            }
        }
    }
}

double SolarCalculator::normalizeDegrees(double angle) {
    angle = std::fmod(angle, 360.0);

    if (angle < 0)
        angle += 360.0;

    return angle;
}

int SolarCalculator::dayOfYear(
    int year, int month, int day
) {
    int result = day;

    for (int m = 1; m < month; m++)
        result += daysInMonth(year, m);

    return result;
}

SolarPosition SolarCalculator::calculate(
    const SolarInput& input
) {
    if (input.year < 1900 || input.year > 2100 ||
        input.month < 1 || input.month > 12 ||
        input.hour < 0 || input.hour > 23 ||
        input.minute < 0 || input.minute > 59 ||
        input.second < 0 || input.second > 59 ||
        input.latitude < -90 || input.latitude > 90 ||
        input.longitude < -180 || input.longitude > 180 ||
        input.utcOffsetHours < -12 ||
        input.utcOffsetHours > 14) {
        throw std::invalid_argument("Invalid solar input");
    }

    if (input.day < 1 ||
        input.day > daysInMonth(input.year, input.month)) {
        throw std::invalid_argument("Invalid calendar date");
    }

    int year = input.year;
    int month = input.month;
    int day = input.day;

    double utcMinutes =
        input.hour * 60.0 +
        input.minute +
        input.second / 60.0 -
        input.utcOffsetHours * 60.0;

    // Correct UTC date rollover.
    while (utcMinutes < 0) {
        utcMinutes += 1440;
        adjustDate(year, month, day, -1);
    }

    while (utcMinutes >= 1440) {
        utcMinutes -= 1440;
        adjustDate(year, month, day, 1);
    }

    double utcHour = utcMinutes / 60.0;

    int doy = dayOfYear(year, month, day);

    double yearLength = isLeapYear(year) ? 366.0 : 365.0;

    double gamma =
        2.0 * PI / yearLength *
        (doy - 1 + (utcHour - 12.0) / 24.0);

    double eqTime =
        229.18 * (
            0.000075
            + 0.001868 * std::cos(gamma)
            - 0.032077 * std::sin(gamma)
            - 0.014615 * std::cos(2 * gamma)
            - 0.040849 * std::sin(2 * gamma)
        );

    double declination =
        0.006918
        - 0.399912 * std::cos(gamma)
        + 0.070257 * std::sin(gamma)
        - 0.006758 * std::cos(2 * gamma)
        + 0.000907 * std::sin(2 * gamma)
        - 0.002697 * std::cos(3 * gamma)
        + 0.001480 * std::sin(3 * gamma);

    double latitude = toRadians(input.latitude);

    double solarTime = std::fmod(
        utcMinutes + eqTime + 4.0 * input.longitude,
        1440.0
    );

    if (solarTime < 0)
        solarTime += 1440.0;

    double hourAngle =
        toRadians(solarTime / 4.0 - 180.0);

    double cosZenith =
        std::sin(latitude) * std::sin(declination)
        + std::cos(latitude) * std::cos(declination)
        * std::cos(hourAngle);

    cosZenith = std::max(
        -1.0, std::min(1.0, cosZenith)
    );

    double elevation =
        90.0 - toDegrees(std::acos(cosZenith));

    double azimuth = normalizeDegrees(
        toDegrees(std::atan2(
            std::sin(hourAngle),
            std::cos(hourAngle) * std::sin(latitude)
            - std::tan(declination) * std::cos(latitude)
        )) + 180.0
    );

    double az = toRadians(azimuth);
    double el = toRadians(elevation);

    SolarPosition result;

    result.azimuth = azimuth;
    result.elevation = elevation;
    result.declination = toDegrees(declination);
    result.equationOfTime = eqTime;
    result.hourAngle = toDegrees(hourAngle);

    result.directionX = std::cos(el) * std::sin(az);
    result.directionY = std::sin(el);
    result.directionZ = std::cos(el) * std::cos(az);

    result.isAboveHorizon = elevation > 0.0;

    return result;
}
