#include "CityDatabase.h"
#include <QFile>
#include <QIODevice>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

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
