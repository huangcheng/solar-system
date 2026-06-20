#include "TerminatorModel.h"
#include <cmath>

QVector3D TerminatorModel::surfaceNormal(double latDeg, double lonDeg) {
    const double la = qDegreesToRadians(latDeg);
    const double lo = qDegreesToRadians(lonDeg);
    const double cl = std::cos(la);
    return QVector3D(float(cl * std::cos(lo)),
                     float(cl * std::sin(lo)),
                     float(std::sin(la)));
}

float TerminatorModel::dayFactor(QVector3D sunDir, double latDeg, double lonDeg) {
    const QVector3D p = surfaceNormal(latDeg, lonDeg).normalized();
    const double cosGeo = double(QVector3D::dotProduct(p, sunDir.normalized()));
    double t = (cosGeo - kTerminatorLow) / (kTerminatorHigh - kTerminatorLow);
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    return float(t * t * (3.0 - 2.0 * t));  // smoothstep
}
