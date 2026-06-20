#include <QtTest>
#include "celestial/ConfigManager.h"
#include <QTemporaryDir>

class TestConfigManager : public QObject { Q_OBJECT
private slots:
    void savesAndLoadsValues() {
        QTemporaryDir dir;
        ConfigManager c(QStringLiteral("earth"), dir.path());
        c.setWindowX(123); c.setWindowY(456);
        c.setDiameter(420); c.setQualityTier(ConfigManager::HD);
        c.setFpsCap(30); c.setLocationOptIn(true); c.setHomeLongitude(116.4);
        c.setShowGrid(true);
        c.setNightMode(QStringLiteral("texture"));
        c.setLanguage(QStringLiteral("zh_CN"));
        c.save();

        ConfigManager c2(QStringLiteral("earth"), dir.path());
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
        ConfigManager c(QStringLiteral("earth"), dir.path());
        c.setDiameter(10);
        QCOMPARE(c.diameter(), 160);
        c.setDiameter(100000);
        QCOMPARE(c.diameter(), 1024);
    }
    void testPathForBodyIsolatesBodies() {
        // Earth keeps the legacy top-level config.json (Phase 1 compat);
        // other bodies each get their own subdir so they cannot clobber it.
        QVERIFY(ConfigManager::pathForBody("earth").endsWith("/config.json"));
        QVERIFY(!ConfigManager::pathForBody("earth").contains("/earth/"));
        QVERIFY(ConfigManager::pathForBody("mars").endsWith("/mars/config.json"));
        QVERIFY(ConfigManager::pathForBody("sun").endsWith("/sun/config.json"));
        QVERIFY(ConfigManager::pathForBody("earth") != ConfigManager::pathForBody("mars"));
    }
};

QTEST_GUILESS_MAIN(TestConfigManager)
#include "test_configmanager.moc"
