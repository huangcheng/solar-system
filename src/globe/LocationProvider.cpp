#include "LocationProvider.h"

LocationProvider::LocationProvider(QObject *parent) : QObject(parent) {}

void LocationProvider::requestPermission() {
#ifdef Q_OS_WIN
    // TODO(impl): WinRT Windows.Devices.Geolocation — set permission on result.
#endif
#ifdef Q_OS_MAC
    // TODO(impl): CoreLocation CLLocationManager.
#endif
#ifdef Q_OS_LINUX
    // TODO(impl): GeoClue2.
#endif
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
