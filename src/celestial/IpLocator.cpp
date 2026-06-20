#include "IpLocator.h"
#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QIODevice>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QUrl>
#include <QTimer>
#include <cmath>

// Pull a double out of a JSON value, accepting both numeric literals and
// numeric strings (some services send coordinates as strings, e.g. ipinfo's
// combined "loc" field once split). Returns false if not convertible.
static bool jsonToDouble(const QJsonValue &v, double &out)
{
    if (v.isDouble()) { out = v.toDouble(); return true; }
    if (v.isString()) {
        bool ok = false;
        out = v.toString().toDouble(&ok);
        return ok;
    }
    return false;
}

QList<IpService> IpLocator::compiledDefault()
{
    // Always-available fallback so the app works even if the loose file is
    // missing or corrupt. ip-api needs no API key and returns plain JSON.
    QList<IpService> out;
    IpService s;
    s.name = QStringLiteral("ip-api");
    s.url = QStringLiteral("http://ip-api.com/json/");
    s.latField = QStringLiteral("lat");
    s.lonField = QStringLiteral("lon");
    s.cityField = QStringLiteral("city");
    s.timeoutSec = 3;
    out.append(s);
    return out;
}

QList<IpService> IpLocator::loadServices()
{
    const QString path =
        QCoreApplication::applicationDirPath() + QStringLiteral("/data/ip-services.json");
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return compiledDefault();   // missing: use compiled-in default
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject())
        return compiledDefault();   // corrupt: fall back
    const QJsonArray arr = doc.object().value(QStringLiteral("services")).toArray();
    if (arr.isEmpty())
        return compiledDefault();   // empty list: fall back
    QList<IpService> out;
    out.reserve(arr.size());
    for (const QJsonValue &v : arr) {
        const QJsonObject o = v.toObject();
        IpService s;
        s.name = o.value("name").toString();
        s.url = o.value("url").toString();
        s.latField = o.value("latField").toString();
        s.lonField = o.value("lonField").toString();
        s.locField = o.value("locField").toString();
        s.locSeparator = o.value("locSeparator").toString();
        s.cityField = o.value("cityField").toString();
        s.timeoutSec = o.value("timeoutSec").toInt(3);
        if (!s.url.isEmpty())
            out.append(std::move(s));
    }
    return out.isEmpty() ? compiledDefault() : out;
}

bool IpLocator::extract(const QJsonDocument &doc, const IpService &s,
                        double &outLat, double &outLon, QString &outCity)
{
    if (!doc.isObject())
        return false;
    const QJsonObject o = doc.object();

    double lat = 0.0, lon = 0.0;
    if (!s.locField.isEmpty()) {
        // Combined "lat,lon" field (e.g. ipinfo's "loc").
        const QStringList parts = o.value(s.locField).toString().split(
            s.locSeparator.isEmpty() ? QStringLiteral(",") : s.locSeparator);
        if (parts.size() < 2)
            return false;
        bool okLat = false, okLon = false;
        lat = parts.at(0).trimmed().toDouble(&okLat);
        lon = parts.at(1).trimmed().toDouble(&okLon);
        if (!okLat || !okLon)
            return false;
    } else {
        if (!jsonToDouble(o.value(s.latField), lat))
            return false;
        if (!jsonToDouble(o.value(s.lonField), lon))
            return false;
    }

    // Sanity-check ranges before accepting.
    if (lat < -90.0 || lat > 90.0)
        return false;
    if (lon < -180.0 || lon > 180.0)
        return false;

    outLat = lat;
    outLon = lon;
    outCity = o.value(s.cityField).toString();
    return true;
}

IpLocator::IpLocator(QObject *parent)
    : QObject(parent)
{
}

void IpLocator::request()
{
    m_services = loadServices();
    m_index = 0;
    tryNext();
}

void IpLocator::tryNext()
{
    if (m_index >= m_services.size()) {
        emit failed();
        return;
    }
    const IpService &s = m_services[m_index];
    QNetworkRequest req((QUrl(s.url)));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    m_reply = m_nam.get(req);

    // Arm a hard timeout: abort only this specific reply. Capturing a
    // QPointer (rather than m_reply) means that if this service's reply was
    // already processed and tryNext() advanced to the next service, the stale
    // timer fires on a null guard and aborts nothing — it can never abort a
    // later service's reply. The resulting finished() signal drives the
    // fallback via onReply, keeping a single advance path.
    QPointer<QNetworkReply> guarded = m_reply;
    QTimer::singleShot(s.timeoutSec * 1000, this, [guarded]() {
        if (guarded)        // null if the reply was already finished/deleteLater'd
            guarded->abort();
    });

    connect(m_reply, &QNetworkReply::finished, this, &IpLocator::onReply);
}

void IpLocator::onReply()
{
    QNetworkReply *reply = m_reply;
    m_reply = nullptr;          // take ownership; guards the timeout lambda
    if (!reply)
        return;                 // already handled

    bool ok = false;
    double lat = 0.0, lon = 0.0;
    QString city;
    if (reply->error() == QNetworkReply::NoError)
        ok = extract(QJsonDocument::fromJson(reply->readAll()),
                     m_services.value(m_index), lat, lon, city);
    reply->deleteLater();

    if (ok) {
        emit located(lat, lon, city);
    } else {
        ++m_index;
        tryNext();
    }
}
