#include <QtTest>
#include "CameraController.h"

class TestCameraController : public QObject { Q_OBJECT
private slots:
    void startsAtZeroOffset() {
        CameraController c;
        QCOMPARE(c.offsetYaw(), 0.0f);
        QCOMPARE(c.offsetPitch(), 0.0f);
    }
    void dragChangesOffset() {
        CameraController c;
        c.applyDrag(100, 40);
        QVERIFY(c.offsetYaw() != 0.0f);
        QVERIFY(c.offsetPitch() != 0.0f);
    }
    void resetClearsOffset() {
        CameraController c;
        c.applyDrag(100, 40);
        c.reset();
        QCOMPARE(c.offsetYaw(), 0.0f);
        QCOMPARE(c.offsetPitch(), 0.0f);
    }
    void zoomClamped() {
        CameraController c;
        c.applyZoom(1000000000);
        QVERIFY(c.zoom() <= CameraController::kMaxZoom);
        c.applyZoom(-1000000000);
        QVERIFY(c.zoom() >= CameraController::kMinZoom);
    }
};

QTEST_GUILESS_MAIN(TestCameraController)
#include "test_cameracontroller.moc"
