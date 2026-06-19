#pragma once
#include <QObject>

// Abstraction over platform geolocation. v1 ships with a mock implementation
// that tests drive directly; Windows (WinRT), macOS (CoreLocation), and Linux
// (GeoClue) hooks are added behind the same interface. Coordinates stay local.
class LocationProvider : public QObject {
    Q_OBJECT
public:
    enum Permission { Unknown = 0, Granted = 1, Denied = 2 };
    Q_ENUM(Permission)

    explicit LocationProvider(QObject *parent = nullptr);

    void requestPermission();
    Permission permission() const { return m_perm; }
    void setPermission(Permission p);

    // Start Qt Positioning (default source). Triggers the OS location-permission
    // prompt; on the first valid fix it sets permission=Granted + coordinates.
    void start();

    bool isEnabled() const;
    double latitude() const { return m_lat; }
    double longitude() const { return m_lon; }
    void setCoordinates(double lat, double lon);

signals:
    void permissionChanged(Permission p);
    void locationChanged(double lat, double lon);

private:
    Permission m_perm = Unknown;
    double m_lat = 0.0, m_lon = 0.0;
    class QGeoPositionInfoSource *m_src = nullptr;
};
