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

// Network IP geolocator. Reads its service list from a loose, editable
// `data/ip-services.json` (shipped next to the executable) and tries each
// service in order until one succeeds. Falls back to a compiled-in minimal
// default (ip-api) if the file is missing or corrupt, so the app always has
// at least one provider — no recompile needed to add or reorder services.
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
