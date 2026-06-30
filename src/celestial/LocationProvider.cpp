#include "LocationProvider.h"
#include <QGeoPositionInfoSource>
#include <QGeoPositionInfo>
#include <QTimer>
#include <QCoreApplication>
#include <QPermissions>

LocationProvider::LocationProvider(QObject *parent) : QObject(parent) {}

// Silent unless GLOBE_DIAG is set in the environment; lets us probe the
// positioning backend without affecting normal runs.
namespace { bool globeDiag() { static const bool v = qEnvironmentVariableIsSet("GLOBE_DIAG"); return v; } }

void LocationProvider::requestPermission() {
    start();
}

// Creates the default positioning source once and wires its signals. This is
// called only after the app has already obtained macOS location permission,
// because since Qt 6.6 the CoreLocation plugin emits AccessError if permission
// is not granted when startUpdates()/requestUpdate() is called.
bool LocationProvider::ensureSource() {
    if (m_src) return true;
    auto *src = QGeoPositionInfoSource::createDefaultSource(this);
    if (globeDiag())
        qWarning() << "[GLOBE_DIAG] createDefaultSource =>"
                   << (src ? src->sourceName() : QStringLiteral("<null>"));
    if (!src) return false;
    m_src = src;
    connect(src, &QGeoPositionInfoSource::positionUpdated, this,
        [this](const QGeoPositionInfo &info) {
            if (globeDiag()) qWarning() << "[GLOBE_DIAG] positionUpdated valid=" << info.isValid();
            if (!info.isValid()) return;
            const QGeoCoordinate c = info.coordinate();
            if (!c.isValid()) return;
            m_pendingOneShot = false;          // got a fix: clear the one-shot wait
            if (m_perm != Granted) setPermission(Granted);
            setCoordinates(c.latitude(), c.longitude());
        });
    connect(src, &QGeoPositionInfoSource::errorOccurred, this,
        [this](QGeoPositionInfoSource::Error err) {
            if (globeDiag()) qWarning() << "[GLOBE_DIAG] errorOccurred" << err;
            // Hard failures mean the OS denied permission or there is no usable
            // backend. Surface them so callers can fall back to IP instead of
            // waiting forever (this was the old "silently does nothing" bug).
            if (err == QGeoPositionInfoSource::AccessError
                || err == QGeoPositionInfoSource::ClosedError
                || err == QGeoPositionInfoSource::UnknownSourceError) {
                m_pendingOneShot = false;
                if (m_perm != Denied) setPermission(Denied);
                emit locationFailed();
            }
        });
    return true;
}

void LocationProvider::doStart() {
    if (!ensureSource()) { emit locationFailed(); return; }
    m_src->setUpdateInterval(60000);
    m_src->startUpdates();
}

void LocationProvider::start() {
    // Qt 6.6+ requires the application to request location permission before
    // the CoreLocation plugin will deliver fixes. Ask the user; only start the
    // source once permission has been granted.
    QLocationPermission perm;
    perm.setAccuracy(QLocationPermission::Precise);
    perm.setAvailability(QLocationPermission::WhenInUse);

    Qt::PermissionStatus status = QCoreApplication::instance()->checkPermission(perm);
    if (status == Qt::PermissionStatus::Granted) {
        if (globeDiag()) qWarning() << "[GLOBE_DIAG] permission already Granted";
        doStart();
    } else if (status == Qt::PermissionStatus::Undetermined) {
        if (globeDiag()) qWarning() << "[GLOBE_DIAG] requesting location permission (start)";
        QCoreApplication::instance()->requestPermission(perm, this, [this](QPermission p) {
            if (globeDiag()) qWarning() << "[GLOBE_DIAG] permission callback (start)" << p.status();
            if (p.status() == Qt::PermissionStatus::Granted) doStart();
            else emit locationFailed();
        });
    } else {
        if (globeDiag()) qWarning() << "[GLOBE_DIAG] permission denied (start)";
        emit locationFailed();
    }
}

void LocationProvider::stop() {
    if (!m_src) return;
    m_src->stopUpdates();
    m_src->deleteLater();   // async-delete: safe even if called from a source signal
    m_src = nullptr;        // lets start() recreate the source on re-enable
}

void LocationProvider::doRequestOnceSystem(int timeoutMs) {
    if (!ensureSource()) {
        emit locationFailed();   // no backend at all: let callers fall back to IP
        return;
    }
    m_pendingOneShot = true;     // cleared on first fix, on a hard error, or by the timer
    m_src->requestUpdate(timeoutMs);
    // Sentinel: if no fix arrives within the timeout, surface a failure so the
    // UI/IP fallback can kick in. We use our own timer rather than relying on
    // Qt's UpdateTimeout error so behavior is identical across Qt versions.
    QTimer::singleShot(timeoutMs, this, [this]() {
        if (m_pendingOneShot) {
            if (globeDiag()) qWarning() << "[GLOBE_DIAG] one-shot timed out, no fix";
            m_pendingOneShot = false;
            emit locationFailed();
        }
    });
}

void LocationProvider::requestOnceSystem(int timeoutMs) {
    // Qt 6.6+ requires the application to request location permission before
    // the CoreLocation plugin will deliver fixes. Ask the user; only request a
    // GPS fix once permission has been granted.
    QLocationPermission perm;
    perm.setAccuracy(QLocationPermission::Precise);
    perm.setAvailability(QLocationPermission::WhenInUse);

    Qt::PermissionStatus status = QCoreApplication::instance()->checkPermission(perm);
    if (status == Qt::PermissionStatus::Granted) {
        if (globeDiag()) qWarning() << "[GLOBE_DIAG] permission already Granted";
        doRequestOnceSystem(timeoutMs);
    } else if (status == Qt::PermissionStatus::Undetermined) {
        if (globeDiag()) qWarning() << "[GLOBE_DIAG] requesting location permission (one-shot)";
        QCoreApplication::instance()->requestPermission(perm, this,
            [this, timeoutMs](QPermission p) {
                if (globeDiag()) qWarning() << "[GLOBE_DIAG] permission callback (one-shot)" << p.status();
                if (p.status() == Qt::PermissionStatus::Granted)
                    doRequestOnceSystem(timeoutMs);
                else
                    emit locationFailed();
            });
    } else {
        if (globeDiag()) qWarning() << "[GLOBE_DIAG] permission denied (one-shot)";
        emit locationFailed();
    }
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
