#include "LocationProvider.h"
#include <QGeoPositionInfoSource>
#include <QGeoPositionInfo>

LocationProvider::LocationProvider(QObject *parent) : QObject(parent) {}

void LocationProvider::requestPermission() {
    start();  // Qt Positioning handles the OS permission prompt
}

void LocationProvider::start() {
    if (m_src) return;
    auto *src = QGeoPositionInfoSource::createDefaultSource(this);
    if (!src) return;
    m_src = src;
    connect(src, &QGeoPositionInfoSource::positionUpdated, this,
        [this](const QGeoPositionInfo &info) {
            if (!info.isValid()) return;
            const QGeoCoordinate c = info.coordinate();
            if (!c.isValid()) return;
            if (m_perm != Granted) setPermission(Granted);
            setCoordinates(c.latitude(), c.longitude());
        });
    src->setUpdateInterval(60000);
    src->startUpdates();
}

void LocationProvider::stop() {
    if (!m_src) return;
    m_src->stopUpdates();
    m_src->deleteLater();   // async-delete: safe even if called from a source signal
    m_src = nullptr;        // lets start() recreate the source on re-enable
}

void LocationProvider::setPermission(Permission p) {
    if (m_perm == p) return;
    m_perm = p;
    emit permissionChanged(p);
}

bool LocationProvider::isEnabled() const {
    return m_perm == Granted;
}

void LocationProvider::setCoordinates(double lat, double lon) {
    m_lat = lat; m_lon = lon;
    emit locationChanged(lat, lon);
}
