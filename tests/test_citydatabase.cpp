#include <QtTest/QtTest>
#include "celestial/CityDatabase.h"
#include <QJsonDocument>
#include <QJsonArray>

class TestCityDatabase : public QObject {
    Q_OBJECT
private slots:
    void parseCanned();
    void resourceLoads();
    void findTokyo();
};

void TestCityDatabase::parseCanned() {
    auto arr = QJsonDocument::fromJson("[{\"name\":\"X\",\"country\":\"Y\",\"lat\":1.5,\"lon\":2.5}]").array();
    auto v = CityDatabase::parse(arr);
    QCOMPARE(v.size(), 1);
    QCOMPARE(v[0].name, QStringLiteral("X"));
    QCOMPARE(v[0].lat, 1.5);
    QCOMPARE(v[0].display(), QStringLiteral("X, Y"));
}

void TestCityDatabase::resourceLoads() {
    QVERIFY(CityDatabase::instance().all().size() > 1000);   // bundled ~1269
}

void TestCityDatabase::findTokyo() {
    auto c = CityDatabase::instance().findByDisplay(QStringLiteral("Tokyo, Japan"));
    QVERIFY(c.has_value());
    QVERIFY(c->lat > 35.0 && c->lat < 36.0);
    QVERIFY(c->lon > 139.0 && c->lon < 140.0);
}

QTEST_MAIN(TestCityDatabase)
#include "test_citydatabase.moc"
