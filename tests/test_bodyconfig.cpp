#include <QtTest/QtTest>
#include <QList>
#include <QSet>
#include "celestial/BodyConfig.h"

class TestBodyConfig : public QObject {
    Q_OBJECT
private slots:
    void earthDefaults();
    void allFactoriesProduceValidConfig();
    void bodyIdsAreUnique();
    void sunIsEmissive();
    void saturnHasRingsAndRingTexture();
    void planetsHaveOrbitalElements();
    void sunAndMoonHaveNoOrbit();
};

void TestBodyConfig::earthDefaults() {
    BodyConfig c = BodyConfig::earth();
    QCOMPARE(c.bodyId, QStringLiteral("earth"));
    QCOMPARE(c.displayName, QStringLiteral("Earth"));
    QCOMPARE(c.rotationPeriodHours, 23.9344696f);
    QVERIFY(c.hasAtmosphere);
}

void TestBodyConfig::allFactoriesProduceValidConfig() {
    QList<BodyConfig> all = {
        BodyConfig::sun(), BodyConfig::moon(), BodyConfig::mercury(),
        BodyConfig::venus(), BodyConfig::earth(), BodyConfig::mars(),
        BodyConfig::jupiter(), BodyConfig::saturn(),
        BodyConfig::uranus(), BodyConfig::neptune(),
    };
    QCOMPARE(all.size(), 10);
    for (const BodyConfig& c : all) {
        QVERIFY2(!c.bodyId.isEmpty(), qPrintable(c.displayName + " missing bodyId"));
        QVERIFY2(!c.displayName.isEmpty(), qPrintable(c.bodyId + " missing displayName"));
        QVERIFY2(!c.albedoTexture.isEmpty(), qPrintable(c.bodyId + " missing albedo"));
        QVERIFY2(c.radiusKm > 0.0f, qPrintable(c.bodyId + " bad radius"));
        QVERIFY2(c.semiMajorAxisAU >= 0.0f, qPrintable(c.bodyId + " negative semi-major axis"));
        QVERIFY2(c.eccentricity >= 0.0f && c.eccentricity < 1.0f,
                 qPrintable(c.bodyId + " eccentricity out of [0,1)"));
    }
}

void TestBodyConfig::bodyIdsAreUnique() {
    QList<BodyConfig> all = {
        BodyConfig::sun(), BodyConfig::moon(), BodyConfig::mercury(),
        BodyConfig::venus(), BodyConfig::earth(), BodyConfig::mars(),
        BodyConfig::jupiter(), BodyConfig::saturn(),
        BodyConfig::uranus(), BodyConfig::neptune(),
    };
    QSet<QString> ids;
    for (const BodyConfig& c : all) {
        QVERIFY2(!ids.contains(c.bodyId), qPrintable("duplicate bodyId: " + c.bodyId));
        ids.insert(c.bodyId);
    }
    QCOMPARE(ids.size(), 10);
}

void TestBodyConfig::sunIsEmissive() {
    QVERIFY(BodyConfig::sun().emissive);
}

void TestBodyConfig::saturnHasRingsAndRingTexture() {
    BodyConfig c = BodyConfig::saturn();
    QVERIFY(c.hasRings);
    QVERIFY(!c.ringTexture.isEmpty());
}

void TestBodyConfig::planetsHaveOrbitalElements() {
    // All planets (not Sun/Moon) have a non-zero semi-major axis.
    for (BodyConfig c : {BodyConfig::mercury(), BodyConfig::venus(), BodyConfig::mars(),
                         BodyConfig::jupiter(), BodyConfig::saturn(),
                         BodyConfig::uranus(), BodyConfig::neptune()}) {
        QVERIFY2(c.semiMajorAxisAU > 0.0f, qPrintable(c.bodyId + " missing a"));
    }
}

void TestBodyConfig::sunAndMoonHaveNoOrbit() {
    QCOMPARE(BodyConfig::sun().semiMajorAxisAU, 0.0f);
    QCOMPARE(BodyConfig::moon().semiMajorAxisAU, 0.0f);
}

QTEST_MAIN(TestBodyConfig)
#include "test_bodyconfig.moc"
