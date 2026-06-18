#include <QtTest>
#include <QVector3D>
#include <QtMath>
#include <cmath>
#include "GlobeRenderer.h"

// Regression: the sun-centric base rotation must map the sub-solar point
// (SunModel's sun direction) onto +Z for ANY (lat, lon), so the lit
// hemisphere always faces the camera. A previous version reversed the
// QMatrix4x4::rotate multiplication order and only worked at Greenwich noon.
class TestSunView : public QObject { Q_OBJECT
private slots:
    void mapsSubsolarPointToPlusZ_data() {
        QTest::addColumn<double>("lat");
        QTest::addColumn<double>("lon");
        QTest::newRow("greenwich noon") << 0.0 << 0.0;
        QTest::newRow("90E equator") << 0.0 << 90.0;
        QTest::newRow("180 far side") << 0.0 << 180.0;
        QTest::newRow("120W 45N") << 45.0 << -120.0;
        QTest::newRow("june solstice lon") << 23.44 << 121.5;
        QTest::newRow("dec solstice lon") << -23.44 << -61.0;
    }
    void mapsSubsolarPointToPlusZ() {
        QFETCH(double, lat);
        QFETCH(double, lon);

        const float lr = qDegreesToRadians(float(lat));
        const float nr = qDegreesToRadians(float(lon));
        const QVector3D sun(std::cos(lr) * std::cos(nr),
                            std::cos(lr) * std::sin(nr),
                            std::sin(lr));

        const QMatrix4x4 m = GlobeRenderer::sunCentricBaseRotation(lat, lon);
        const QVector3D mapped = m.map(sun);

        QVERIFY2(std::abs(mapped.x()) <= 1e-3f, qPrintable(QString("x=%1").arg(mapped.x())));
        QVERIFY2(std::abs(mapped.y()) <= 1e-3f, qPrintable(QString("y=%1").arg(mapped.y())));
        QVERIFY2(mapped.z() > 0.999f, qPrintable(QString("z=%1").arg(mapped.z())));
    }
};

QTEST_GUILESS_MAIN(TestSunView)
#include "test_sunview.moc"
