
#pragma once

struct SolarInput {
    int year = 2026;
    int month = 6;
    int day = 21;

    int hour = 12;
    int minute = 0;
    int second = 0;

    double latitude = 12.9716;
    double longitude = 77.5946;
    double utcOffsetHours = 5.5;
};

struct SolarPosition {
    double azimuth = 0.0;
    double elevation = 0.0;

    double declination = 0.0;
    double equationOfTime = 0.0;
    double hourAngle = 0.0;

    double directionX = 0.0;
    double directionY = 0.0;
    double directionZ = 0.0;

    bool isAboveHorizon = false;
};

class SolarCalculator {
public:
    static SolarPosition calculate(
        const SolarInput& input
    );

private:
    static int dayOfYear(
        int year, int month, int day
    );

    static double normalizeDegrees(double angle);
};
