#pragma once
#include <QString>

struct BodyConfig {
    QString bodyId;
    QString displayName;
    QString albedoTexture;
    QString nightTexture;
    QString normalTexture;
    QString specularTexture;
    QString cloudTexture;
    QString ringTexture;        // optional (Saturn)

    float radiusKm = 6371.0f;
    float visualScale = 1.0f;
    float rotationPeriodHours = 24.0f;  // sidereal; negative = retrograde rotation
    float axialTiltDegrees = 23.5f;
    bool emissive = false;
    bool hasRings = false;
    bool hasClouds = false;
    bool hasAtmosphere = false;

    // Heliocentric orbital elements (planets only; 0 for Sun/Moon-Earth).
    float semiMajorAxisAU = 0.0f;
    float eccentricity = 0.0f;
    float inclinationDeg = 0.0f;
    float longitudeAscendingNodeDeg = 0.0f;
    float argumentOfPerihelionDeg = 0.0f;
    float meanAnomalyAtEpochDeg = 0.0f;

    static BodyConfig sun();
    static BodyConfig moon();
    static BodyConfig mercury();
    static BodyConfig venus();
    static BodyConfig earth();
    static BodyConfig mars();
    static BodyConfig jupiter();
    static BodyConfig saturn();
    static BodyConfig uranus();
    static BodyConfig neptune();
};
