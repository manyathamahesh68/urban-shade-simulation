
#pragma once

#include "solar_position.h"

class SolarRenderer {
public:

    // Draw the moving sun.
    static void drawSun(
        const SolarPosition& sun,
        float centerX,
        float centerY,
        float centerZ,
        float radius
    );

    // Draw the complete daily sun trajectory.
    static void drawSunPath(
        const SolarInput& input,
        float centerX,
        float centerY,
        float centerZ,
        float radius
    );

    // Update OpenGL directional lighting.
    static void updateLighting(
        const SolarPosition& sun
    );

};
