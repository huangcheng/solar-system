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
    // Stop receiving updates and release the positioning source. Safe to call
    // when not started; lets a subsequent start() recreate the source.
    void stop();

    // One-shot system-location request (single fix, not continuous updates).
    // Used by the startup auto-detect and the Settings "Detect -> System" button.
    // Lazily creates the positioning source on first use; if no backend exists
    // it emits locationFailed() so callers can fall back to IP geolocation.
    void requestOnceSystem(int timeoutMs = 8000);

    bool isEnabled() const;
    double latitude() const { return m_lat; }
    double longitude() const { return m_lon; }
    void setCoordinates(double lat, double lon);

signals:
    void permissionChanged(Permission p);
    void locationChanged(double lat, double lon);
    // Emitted when a system location request cannot produce a fix (permission
    // denied, no usable backend, or one-shot timeout). Lets callers surface the
    // failure and/or fall back to IP geolocation instead of hanging silently.
    void locationFailed();

private:
    Permission m_perm = Unknown;
    double m_lat = 0.0, m_lon = 0.0;
    bool m_pendingOneShot = false;   // true between requestOnceSystem() and first fix/timeout
    class QGeoPositionInfoSource *m_src = nullptr;
    // Create + wire the default positioning source once. Only called after
    // macOS location permission has been granted (Qt 6.6+ requirement).
    bool ensureSource();
    // Actually start continuous updates or a one-shot after permission is known.
    void doStart();
    void doRequestOnceSystem(int timeoutMs);
};
