#include "BodyConfig.h"

BodyConfig BodyConfig::earth() {
    BodyConfig c;
    c.bodyId = QStringLiteral("earth");
    c.displayName = QStringLiteral("Earth");
    // File names match AssetManager::fileName(Slot) so the existing loader
    // picks up the textures currently shipped in ./textures without changes.
    c.albedoTexture   = QStringLiteral("day.jpg");
    c.nightTexture    = QStringLiteral("night.jpg");
    c.normalTexture   = QStringLiteral("normal.png");
    c.specularTexture = QStringLiteral("specular.png");
    c.cloudTexture    = QStringLiteral("clouds.png");
    c.radiusKm = 6371.0f;
    c.visualScale = 1.0f;
    c.rotationPeriodHours = 23.9344696f;
    c.axialTiltDegrees = 23.5f;
    c.hasClouds = false;   // currently disabled
    c.hasAtmosphere = true;
    return c;
}
