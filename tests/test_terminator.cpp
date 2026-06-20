#include <QtTest/QtTest>
#include "celestial/TerminatorModel.h"

class TestTerminator : public QObject {
    Q_OBJECT
private slots:
    void subSolarPointIsFullDay();
    void antipodeIsFullNight();
    void orthogonalIsMid();
};

void TestTerminator::subSolarPointIsFullDay() {
    QVector3D sun = TerminatorModel::surfaceNormal(0.0, 0.0);
    float f = TerminatorModel::dayFactor(sun, 0.0, 0.0);
    QVERIFY(f > 0.99f);
}

void TestTerminator::antipodeIsFullNight() {
    QVector3D sun = TerminatorModel::surfaceNormal(0.0, 0.0);
    float f = TerminatorModel::dayFactor(sun, 0.0, 180.0);
    QVERIFY(f < 0.01f);
}

void TestTerminator::orthogonalIsMid() {
    QVector3D sun = TerminatorModel::surfaceNormal(0.0, 0.0);
    float f = TerminatorModel::dayFactor(sun, 0.0, 90.0);
    QVERIFY(f > 0.2f && f < 0.8f);
}

QTEST_MAIN(TestTerminator)
#include "test_terminator.moc"
