#pragma once
#include <QString>
#include <QList>
#include <optional>

struct CityEntry {
    QString name, country;
    double lat = 0.0, lon = 0.0;
    QString display() const { return name + QStringLiteral(", ") + country; }
};

// Lazily-loaded, process-wide table of bundled world cities (:/data/cities.json).
// Used by the location-selector UI; the singleton loads the resource once.
class CityDatabase {
public:
    static CityDatabase &instance();   // loads :/data/cities.json once (lazy)
    const QList<CityEntry> &all() const { return m_cities; }
    std::optional<CityEntry> findByDisplay(const QString &display) const;  // "City, Country"
    std::optional<CityEntry> findNearest(double lat, double lon) const;    // closest city by Haversine
    // Pure parse helper (testable without the resource):
    static QList<CityEntry> parse(const class QJsonArray &arr);
private:
    CityDatabase();
    QList<CityEntry> m_cities;
};
