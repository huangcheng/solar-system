#include <QtTest>
#include "LocationProvider.h"

class TestLocationProvider : public QObject { Q_OBJECT
private slots:
    void deniedDisablesLocation() {
        LocationProvider p;
        p.setPermission(LocationProvider::Denied);
        QVERIFY(!p.isEnabled());
    }
    void grantedWithCoordinatesEnables() {
        LocationProvider p;
        p.setPermission(LocationProvider::Granted);
        p.setCoordinates(39.9, 116.4);
        QVERIFY(p.isEnabled());
        QCOMPARE(p.latitude(), 39.9);
        QCOMPARE(p.longitude(), 116.4);
    }
};

QTEST_GUILESS_MAIN(TestLocationProvider)
#include "test_locationprovider.moc"
