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

    float radiusKm = 6371.0f;
    float visualScale = 1.0f;
    float rotationPeriodHours = 24.0f;
    float axialTiltDegrees = 23.5f;
    bool emissive = false;
    bool hasRings = false;
    bool hasClouds = false;
    bool hasAtmosphere = false;

    static BodyConfig earth();
};
