#pragma once
#include <QObject>
#include <QString>
#include <QList>
#include <QNetworkAccessManager>

struct IpService {
    QString name, url, latField, lonField, locField, locSeparator, cityField;
    int timeoutSec = 3;
};

class QNetworkReply;

// Network IP geolocator. Loads its service list in priority order: a loose,
// editable <appDir>/data/ip-services.json (if present), then an embedded
// :/data/ip-services.json resource (always shipped in the binary, so it works
// in bundled/installed/DMG builds), then a hard-coded HTTPS-only fallback.
// Services are tried in order until one succeeds. All built-in services use
// HTTPS because macOS App Transport Security blocks plain HTTP.
class IpLocator : public QObject {
    Q_OBJECT
public:
    explicit IpLocator(QObject *parent = nullptr);
    void request();   // try services in order from data/ip-services.json (fallback: compiled-in default)
signals:
    void located(double lat, double lon, const QString &city);
    void failed();
private:
    QNetworkAccessManager m_nam;
    QList<IpService> m_services;
    int m_index = 0;
    QNetworkReply *m_reply = nullptr;
    void tryNext();
    void onReply();
    // pure, testable:
    static QList<IpService> loadServices();        // appDir/data/ip-services.json -> compiled-in fallback
public:
    // Always-available provider list used when the loose file is missing/corrupt.
    static QList<IpService> compiledDefault();
    // Testable extraction: returns true + fills outLat/outLon/outCity on success.
    static bool extract(const class QJsonDocument &doc, const IpService &s,
                        double &outLat, double &outLon, QString &outCity);
};
