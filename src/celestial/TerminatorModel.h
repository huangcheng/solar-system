#pragma once
#include <QVector3D>

// GL-free day/night terminator model. Given the ECEF unit sun direction and a
// surface point by latitude/longitude, returns an illumination factor in
// [0,1]: 1 = full day, 0 = full night, ~0.5 near the terminator. This is the
// CPU-side twin of map.frag's per-pixel math; keeping them in sync guarantees
// the flat-map terminator is unit-testable.
class TerminatorModel {
public:
    static float dayFactor(QVector3D sunDir, double latDeg, double lonDeg);
    static QVector3D surfaceNormal(double latDeg, double lonDeg);
    static constexpr double kTerminatorLow = -0.10;
    static constexpr double kTerminatorHigh = 0.20;
};
