#include "BodyConfig.h"

BodyConfig BodyConfig::sun() {
    BodyConfig c;
    c.bodyId = QStringLiteral("sun");
    c.displayName = QStringLiteral("Sun");
    c.albedoTexture = QStringLiteral("sun.jpg");
    c.radiusKm = 696340.0f;
    c.rotationPeriodHours = 609.12f;   // ~25.4 days
    c.axialTiltDegrees = 7.25f;
    c.emissive = true;
    return c;
}

BodyConfig BodyConfig::moon() {
    BodyConfig c;
    c.bodyId = QStringLiteral("moon");
    c.displayName = QStringLiteral("Moon");
    c.albedoTexture = QStringLiteral("moon.jpg");
    c.radiusKm = 1737.4f;
    c.rotationPeriodHours = 655.72f;   // tidally locked
    c.axialTiltDegrees = 6.68f;
    return c;
}

BodyConfig BodyConfig::mercury() {
    BodyConfig c;
    c.bodyId = QStringLiteral("mercury"); c.displayName = QStringLiteral("Mercury");
    c.albedoTexture = QStringLiteral("mercury.jpg");
    c.radiusKm = 2439.7f; c.rotationPeriodHours = 1407.6f; c.axialTiltDegrees = 0.03f;
    c.semiMajorAxisAU = 0.387f; c.eccentricity = 0.2056f;
    c.inclinationDeg = 7.0f; c.longitudeAscendingNodeDeg = 48.3f;
    c.argumentOfPerihelionDeg = 29.1f; c.meanAnomalyAtEpochDeg = 174.8f;
    return c;
}

BodyConfig BodyConfig::venus() {
    BodyConfig c;
    c.bodyId = QStringLiteral("venus"); c.displayName = QStringLiteral("Venus");
    c.albedoTexture = QStringLiteral("venus.jpg");
    c.radiusKm = 6051.8f; c.rotationPeriodHours = -5832.5f; c.axialTiltDegrees = 177.4f;  // retrograde
    c.hasAtmosphere = true;
    c.semiMajorAxisAU = 0.723f; c.eccentricity = 0.0068f;
    c.inclinationDeg = 3.4f; c.longitudeAscendingNodeDeg = 76.7f;
    c.argumentOfPerihelionDeg = 54.9f; c.meanAnomalyAtEpochDeg = 50.4f;
    return c;
}

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

BodyConfig BodyConfig::mars() {
    BodyConfig c;
    c.bodyId = QStringLiteral("mars"); c.displayName = QStringLiteral("Mars");
    c.albedoTexture = QStringLiteral("mars.jpg");
    c.radiusKm = 3389.5f; c.rotationPeriodHours = 24.62f; c.axialTiltDegrees = 25.19f;
    c.semiMajorAxisAU = 1.524f; c.eccentricity = 0.0934f;
    c.inclinationDeg = 1.85f; c.longitudeAscendingNodeDeg = 49.6f;
    c.argumentOfPerihelionDeg = 286.5f; c.meanAnomalyAtEpochDeg = 19.4f;
    return c;
}

BodyConfig BodyConfig::jupiter() {
    BodyConfig c;
    c.bodyId = QStringLiteral("jupiter"); c.displayName = QStringLiteral("Jupiter");
    c.albedoTexture = QStringLiteral("jupiter.jpg");
    c.radiusKm = 69911.0f; c.rotationPeriodHours = 9.93f; c.axialTiltDegrees = 3.13f;
    c.hasAtmosphere = true;
    c.semiMajorAxisAU = 5.203f; c.eccentricity = 0.0489f;
    c.inclinationDeg = 1.3f; c.longitudeAscendingNodeDeg = 100.5f;
    c.argumentOfPerihelionDeg = 273.9f; c.meanAnomalyAtEpochDeg = 20.0f;
    return c;
}

BodyConfig BodyConfig::saturn() {
    BodyConfig c;
    c.bodyId = QStringLiteral("saturn"); c.displayName = QStringLiteral("Saturn");
    c.albedoTexture = QStringLiteral("saturn.jpg");
    c.ringTexture = QStringLiteral("saturn_ring.png");
    c.radiusKm = 58232.0f; c.rotationPeriodHours = 10.66f; c.axialTiltDegrees = 26.73f;
    c.hasRings = true; c.hasAtmosphere = true;
    c.semiMajorAxisAU = 9.537f; c.eccentricity = 0.0565f;
    c.inclinationDeg = 2.49f; c.longitudeAscendingNodeDeg = 113.7f;
    c.argumentOfPerihelionDeg = 339.1f; c.meanAnomalyAtEpochDeg = 317.0f;
    return c;
}

BodyConfig BodyConfig::uranus() {
    BodyConfig c;
    c.bodyId = QStringLiteral("uranus"); c.displayName = QStringLiteral("Uranus");
    c.albedoTexture = QStringLiteral("uranus.jpg");
    c.radiusKm = 25362.0f; c.rotationPeriodHours = -17.24f; c.axialTiltDegrees = 97.77f;  // retrograde
    c.hasAtmosphere = true;
    c.semiMajorAxisAU = 19.19f; c.eccentricity = 0.0457f;
    c.inclinationDeg = 0.77f; c.longitudeAscendingNodeDeg = 74.0f;
    c.argumentOfPerihelionDeg = 96.8f; c.meanAnomalyAtEpochDeg = 142.2f;
    return c;
}

BodyConfig BodyConfig::neptune() {
    BodyConfig c;
    c.bodyId = QStringLiteral("neptune"); c.displayName = QStringLiteral("Neptune");
    c.albedoTexture = QStringLiteral("neptune.jpg");
    c.radiusKm = 24622.0f; c.rotationPeriodHours = 16.11f; c.axialTiltDegrees = 28.32f;
    c.hasAtmosphere = true;
    c.semiMajorAxisAU = 30.07f; c.eccentricity = 0.0113f;
    c.inclinationDeg = 1.77f; c.longitudeAscendingNodeDeg = 131.8f;
    c.argumentOfPerihelionDeg = 276.3f; c.meanAnomalyAtEpochDeg = 256.2f;
    return c;
}
