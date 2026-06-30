#include <QtTest/QtTest>
#include "celestial/IpLocator.h"
#include <QJsonDocument>

class TestIpLocator : public QObject {
    Q_OBJECT
private slots:
    void extractIpApi();
    void extractIpinfoLoc();
    void extractIpapi();
    void badJsonReturnsFalse();
    void fallbackNonEmpty();
    void compiledDefaultIsHttps();
};

void TestIpLocator::extractIpApi() {
    auto d = QJsonDocument::fromJson("{\"lat\":35.0,\"lon\":139.0,\"city\":\"Tokyo\"}");
    IpService s; s.latField="lat"; s.lonField="lon"; s.cityField="city";
    double la,lo; QString c;
    QVERIFY(IpLocator::extract(d, s, la, lo, c));
    QCOMPARE(la, 35.0); QCOMPARE(lo, 139.0); QCOMPARE(c, QStringLiteral("Tokyo"));
}

void TestIpLocator::extractIpinfoLoc() {
    auto d = QJsonDocument::fromJson("{\"loc\":\"40.69,-73.92\",\"city\":\"New York\"}");
    IpService s; s.locField="loc"; s.locSeparator=","; s.cityField="city";
    double la,lo; QString c;
    QVERIFY(IpLocator::extract(d, s, la, lo, c));
    QVERIFY(la > 40.0 && la < 41.0);
}

void TestIpLocator::extractIpapi() {
    auto d = QJsonDocument::fromJson("{\"latitude\":39.9,\"longitude\":116.4,\"city\":\"Beijing\"}");
    IpService s; s.latField="latitude"; s.lonField="longitude"; s.cityField="city";
    double la,lo; QString c;
    QVERIFY(IpLocator::extract(d, s, la, lo, c));
    QVERIFY(lo > 116.0 && lo < 117.0);
}

void TestIpLocator::badJsonReturnsFalse() {
    auto d = QJsonDocument::fromJson("{\"foo\":1}");
    IpService s; s.latField="lat"; s.lonField="lon";
    double la,lo; QString c;
    QVERIFY(!IpLocator::extract(d, s, la, lo, c));
}

void TestIpLocator::fallbackNonEmpty() {
    QVERIFY(!IpLocator::compiledDefault().isEmpty());
}

void TestIpLocator::compiledDefaultIsHttps() {
    // Regression guard: macOS App Transport Security blocks plain HTTP, so every
    // built-in fallback service MUST be https:// or IP lookup fails on macOS.
    const auto services = IpLocator::compiledDefault();
    QVERIFY(!services.isEmpty());
    for (const IpService &s : services)
        QVERIFY2(s.url.startsWith(QStringLiteral("https://")),
                 qPrintable(QStringLiteral("Non-HTTPS built-in service blocked by macOS ATS: %1 -> %2")
                                .arg(s.name, s.url)));
}

QTEST_MAIN(TestIpLocator)
#include "test_iplocator.moc"
