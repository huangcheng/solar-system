#include "CityDatabase.h"
#include <QFile>
#include <QIODevice>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <cmath>

namespace {
constexpr double PI = 3.14159265358979323846;
}

QList<CityEntry> CityDatabase::parse(const QJsonArray &arr) {
    QList<CityEntry> out;
    out.reserve(arr.size());
    for (const QJsonValue &v : arr) {
        const QJsonObject o = v.toObject();
        CityEntry e;
        e.name = o.value("name").toString();
        e.country = o.value("country").toString();
        e.lat = o.value("lat").toDouble();
        e.lon = o.value("lon").toDouble();
        out.append(std::move(e));
    }
    return out;
}

CityDatabase::CityDatabase() {
    // Force the bundled resource to be registered even when this code lives in
    // a STATIC library (some linkers GC the rcc object if nothing references it).
    Q_INIT_RESOURCE(cities);

    QFile f(QStringLiteral(":/data/cities.json"));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return;  // resource missing/unreadable: leave m_cities empty (no crash)
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (doc.isArray())
        m_cities = parse(doc.array());
}

CityDatabase &CityDatabase::instance() {
    static CityDatabase db;   // Meyers singleton — constructed once, thread-safe (C++11+)
    return db;
}

std::optional<CityEntry> CityDatabase::findByDisplay(const QString &display) const {
    for (const CityEntry &e : m_cities)
        if (e.display() == display)
            return e;
    return std::nullopt;
}

std::optional<CityEntry> CityDatabase::findNearest(double lat, double lon) const {
    if (m_cities.isEmpty())
        return std::nullopt;
    const auto haversine = [](double aLat, double aLon, double bLat, double bLon) {
        const double D2R = PI / 180.0;
        const double dLat = (bLat - aLat) * D2R;
        const double dLon = (bLon - aLon) * D2R;
        const double sLat = std::sin(dLat / 2.0);
        const double sLon = std::sin(dLon / 2.0);
        double a = sLat * sLat +
                   std::cos(aLat * D2R) * std::cos(bLat * D2R) * sLon * sLon;
        if (a > 1.0) a = 1.0;
        if (a < 0.0) a = 0.0;
        return 2.0 * std::asin(std::sqrt(a));
    };

    const CityEntry *best = &m_cities.first();
    double bestDist = haversine(lat, lon, best->lat, best->lon);
    for (const CityEntry &e : m_cities) {
        const double d = haversine(lat, lon, e.lat, e.lon);
        if (d < bestDist) {
            bestDist = d;
            best = &e;
        }
    }
    return *best;
}
