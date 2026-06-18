#include "SunModel.h"
#include <cmath>

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kDeg2Rad = kPi / 180.0;
constexpr double kRad2Deg = 180.0 / kPi;
}

double SunModel::toRadians(double d) { return d * kDeg2Rad; }
double SunModel::toDegrees(double r) { return r * kRad2Deg; }
double SunModel::wrap180(double d) {
    while (d > 180.0) d -= 360.0;
    while (d < -180.0) d += 360.0;
    return d;
}

double SunModel::julianDay() const {
    const qint64 ms = m_utc.toMSecsSinceEpoch();
    return 2440587.5 + (ms / 86400000.0);
}

double SunModel::subSolarLatitude() const {
    const double n = julianDay() - 2451545.0;
    const double L = wrap180(280.460 + 0.9856474 * n);        // mean longitude
    const double g = toRadians(wrap180(357.528 + 0.9856003 * n)); // mean anomaly
    const double lambda = toRadians(L + 1.915 * std::sin(g) + 0.020 * std::sin(2.0 * g));
    const double eps = toRadians(23.439 - 0.0000004 * n);     // obliquity
    return toDegrees(std::asin(std::sin(eps) * std::sin(lambda))); // declination
}

double SunModel::subSolarLongitude() const {
    const double n = julianDay() - 2451545.0;
    const double L = wrap180(280.460 + 0.9856474 * n);
    const double g = toRadians(wrap180(357.528 + 0.9856003 * n));
    const double lambda = L + 1.915 * std::sin(g) + 0.020 * std::sin(2.0 * g); // ecliptic lon (deg)
    const double eps = toRadians(23.439 - 0.0000004 * n);
    const double ra = toDegrees(std::atan2(std::cos(eps) * std::sin(toRadians(lambda)),
                                           std::cos(toRadians(lambda))));        // right ascension (deg)
    double gmstHours = std::fmod(18.697374558 + 24.06570982441908 * n, 24.0);
    if (gmstHours < 0.0) gmstHours += 24.0;
    return wrap180(ra - gmstHours * 15.0);
}

QVector3D SunModel::sunDirection() const {
    const double lat = toRadians(subSolarLatitude());
    const double lon = toRadians(subSolarLongitude());
    return QVector3D(std::cos(lat) * std::cos(lon),
                     std::cos(lat) * std::sin(lon),
                     std::sin(lat)).normalized();
}
