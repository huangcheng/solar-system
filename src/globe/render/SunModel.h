#pragma once
#include <QDateTime>
#include <QVector3D>

// Computes the sub-solar point and a unit sun direction in Earth-fixed (ECEF)
// coordinates, given a UTC time. Low-precision solar position (~1 deg), enough
// for a visual widget.
class SunModel {
public:
    void setUtc(const QDateTime &utc) { m_utc = utc.toUTC(); }
    const QDateTime &utc() const { return m_utc; }

    double subSolarLatitude() const;   // degrees, roughly [-23.5, 23.5]
    double subSolarLongitude() const;  // degrees, [-180, 180]
    QVector3D sunDirection() const;    // unit, ECEF

private:
    QDateTime m_utc = QDateTime::currentDateTimeUtc();

    static double toRadians(double deg);
    static double toDegrees(double rad);
    static double wrap180(double deg);
    double julianDay() const;
};
