#include <QtTest>
#include "ConfigManager.h"
#include <QTemporaryDir>

class TestConfigManager : public QObject { Q_OBJECT
private slots:
    void savesAndLoadsValues() {
        QTemporaryDir dir;
        ConfigManager c(dir.path());
        c.setWindowX(123); c.setWindowY(456);
        c.setDiameter(420); c.setQualityTier(ConfigManager::HD);
        c.setFpsCap(30); c.setLocationOptIn(true); c.setHomeLongitude(116.4);
        c.save();

        ConfigManager c2(dir.path());
        QCOMPARE(c2.windowX(), 123);
        QCOMPARE(c2.windowY(), 456);
        QCOMPARE(c2.diameter(), 420);
        QCOMPARE(c2.qualityTier(), ConfigManager::HD);
        QCOMPARE(c2.fpsCap(), 30);
        QVERIFY(c2.locationOptIn());
        QCOMPARE(c2.homeLongitude(), 116.4);
    }
    void clampsDiameter() {
        QTemporaryDir dir;
        ConfigManager c(dir.path());
        c.setDiameter(10);
        QCOMPARE(c.diameter(), 160);
        c.setDiameter(100000);
        QCOMPARE(c.diameter(), 1024);
    }
};

QTEST_GUILESS_MAIN(TestConfigManager)
#include "test_configmanager.moc"
