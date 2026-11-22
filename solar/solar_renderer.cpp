
#include "solar_renderer.h"
#include "solar_position.h"

#include <GL/glut.h>
#include <cmath>
#include <algorithm>

void SolarRenderer::drawSun(
    const SolarPosition& sun,
    float cx,
    float cy,
    float cz,
    float radius
) {
    if (!sun.isAboveHorizon)
        return;

    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LIGHTING_BIT);

    glDisable(GL_LIGHTING);

    glPushMatrix();

    glTranslatef(
        cx + static_cast<float>(sun.directionX * radius),
        cy + static_cast<float>(sun.directionY * radius),
        cz + static_cast<float>(sun.directionZ * radius)
    );

    glColor3f(1.0f, 0.85f, 0.15f);

    glutSolidSphere(2.5, 24, 24);

    glPopMatrix();

    glPopAttrib();
}

void SolarRenderer::drawSunPath(
    const SolarInput& input,
    float cx,
    float cy,
    float cz,
    float radius
) {
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT);

    glDisable(GL_LIGHTING);

    glColor3f(1.0f, 0.75f, 0.1f);

    glLineWidth(2.0f);

    glBegin(GL_LINE_STRIP);

    for (int minutes = 0; minutes < 1440; minutes += 5) {

        SolarInput sample = input;

        sample.hour = minutes / 60;
        sample.minute = minutes % 60;
        sample.second = 0;

        SolarPosition sun =
            SolarCalculator::calculate(sample);

        if (!sun.isAboveHorizon)
            continue;

        glVertex3f(
            cx + static_cast<float>(
                sun.directionX * radius
            ),
            cy + static_cast<float>(
                sun.directionY * radius
            ),
            cz + static_cast<float>(
                sun.directionZ * radius
            )
        );
    }

    glEnd();

    glLineWidth(1.0f);

    glPopAttrib();
}

void SolarRenderer::updateLighting(
    const SolarPosition& sun
) {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    if (!sun.isAboveHorizon) {

        GLfloat ambient[] = {
            0.04f, 0.04f, 0.08f, 1.0f
        };

        GLfloat diffuse[] = {
            0.0f, 0.0f, 0.0f, 1.0f
        };

        glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);

        return;
    }

    GLfloat direction[] = {
        static_cast<GLfloat>(sun.directionX),
        static_cast<GLfloat>(sun.directionY),
        static_cast<GLfloat>(sun.directionZ),
        0.0f
    };

    float elevationFactor =
        static_cast<float>(
            std::sin(sun.elevation * 3.14159265 / 180.0)
        );

    elevationFactor = std::max(
        0.15f,
        std::min(1.0f, elevationFactor)
    );

    GLfloat ambient[] = {
        0.18f, 0.18f, 0.20f, 1.0f
    };

    GLfloat diffuse[] = {
        elevationFactor,
        elevationFactor * 0.92f,
        elevationFactor * 0.78f,
        1.0f
    };

    glLightfv(GL_LIGHT0, GL_POSITION, direction);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
}
