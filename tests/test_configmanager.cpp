#include <QtTest>
#include "celestial/ConfigManager.h"
#include <QTemporaryDir>

class TestConfigManager : public QObject { Q_OBJECT
private slots:
    void savesAndLoadsValues() {
        QTemporaryDir dir;
        ConfigManager c(dir.path());
        c.setWindowX(123); c.setWindowY(456);
        c.setDiameter(420); c.setQualityTier(ConfigManager::HD);
        c.setFpsCap(30); c.setLocationOptIn(true); c.setHomeLongitude(116.4);
        c.setShowGrid(true);
        c.setNightMode(QStringLiteral("texture"));
        c.setLanguage(QStringLiteral("zh_CN"));
        c.save();

        ConfigManager c2(dir.path());
        QCOMPARE(c2.windowX(), 123);
        QCOMPARE(c2.windowY(), 456);
        QCOMPARE(c2.diameter(), 420);
        QCOMPARE(c2.qualityTier(), ConfigManager::HD);
        QCOMPARE(c2.fpsCap(), 30);
        QVERIFY(c2.locationOptIn());
        QCOMPARE(c2.homeLongitude(), 116.4);
        QVERIFY(c2.showGrid());
        QCOMPARE(c2.nightMode(), QStringLiteral("texture"));
        QCOMPARE(c2.language(), QStringLiteral("zh_CN"));
    }
    void clampsDiameter() {
        QTemporaryDir dir;
        ConfigManager c(dir.path());
        c.setDiameter(10);
        QCOMPARE(c.diameter(), 160);
        c.setDiameter(100000);
        QCOMPARE(c.diameter(), 1024);
    }
    void testViewModeRoundTrip();
    void testAutoDetectOnStart();
};

void TestConfigManager::testViewModeRoundTrip() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    ConfigManager cm(dir.path());
    QCOMPARE(cm.viewMode(), QStringLiteral("globe"));   // default

    cm.setViewMode(QStringLiteral("map"));
    QCOMPARE(cm.viewMode(), QStringLiteral("map"));
    cm.save();

    ConfigManager cm2(dir.path());
    QCOMPARE(cm2.viewMode(), QStringLiteral("map"));    // persisted

    cm2.setViewMode(QStringLiteral("globe"));
    QCOMPARE(cm2.viewMode(), QStringLiteral("globe"));
    // invalid value clamps to globe
    cm2.setViewMode(QStringLiteral("nonsense"));
    QCOMPARE(cm2.viewMode(), QStringLiteral("globe"));
}

void TestConfigManager::testAutoDetectOnStart() {
    // default: false
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    ConfigManager cm(dir.path());
    QCOMPARE(cm.autoDetectOnStart(), false);

    // set true + round-trip through save/new-load
    cm.setAutoDetectOnStart(true);
    QVERIFY(cm.autoDetectOnStart());
    cm.save();
    {
        ConfigManager loaded(dir.path());
        QVERIFY(loaded.autoDetectOnStart());   // persisted
    }

    // invalid/missing value stays false (fresh dir, no key on disk)
    QTemporaryDir dir2;
    QVERIFY(dir2.isValid());
    ConfigManager cm2(dir2.path());
    QVERIFY(!cm2.autoDetectOnStart());
}

QTEST_GUILESS_MAIN(TestConfigManager)
#include "test_configmanager.moc"
