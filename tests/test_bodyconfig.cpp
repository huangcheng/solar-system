#include <QtTest/QtTest>
#include "BodyConfig.h"

class TestBodyConfig : public QObject {
    Q_OBJECT
private slots:
    void earthDefaults();
};

void TestBodyConfig::earthDefaults() {
    BodyConfig c = BodyConfig::earth();
    QCOMPARE(c.bodyId, QStringLiteral("earth"));
    QCOMPARE(c.displayName, QStringLiteral("Earth"));
    QCOMPARE(c.rotationPeriodHours, 23.9344696f);
    QVERIFY(c.hasAtmosphere);
}

QTEST_MAIN(TestBodyConfig)
#include "test_bodyconfig.moc"
