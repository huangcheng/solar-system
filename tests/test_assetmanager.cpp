#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QPainter>
#include "celestial/AssetManager.h"

class TestAssetManager : public QObject {
    Q_OBJECT
private slots:
    void filenameLookupResolvesExistingFile();
    void filenameLookupFallsBackWhenMissing();
};

void TestAssetManager::filenameLookupResolvesExistingFile() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    // Write a known 100x50 magenta PNG into the dir.
    QImage src(100, 50, QImage::Format_RGB32);
    src.fill(QColor(255, 0, 255));
    QVERIFY(src.save(tmp.path() + "/known.png"));

    AssetManager am(tmp.path());            // explicit dir ctor
    QImage img = am.image(QStringLiteral("known.png"), 4096);
    QVERIFY(!img.isNull());
    QCOMPARE(img.width(), 100);             // real decode, not fallback (fallback is 512)
    QCOMPARE(img.height(), 50);
    QCOMPARE(img.pixel(0, 0), QColor(255, 0, 255).rgb());
}

void TestAssetManager::filenameLookupFallsBackWhenMissing() {
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    AssetManager am(tmp.path());            // empty dir
    QImage img = am.image(QStringLiteral("nope.jpg"), 4096);
    QVERIFY(!img.isNull());
    QCOMPARE(img.width(), 512);             // fallback size
}

QTEST_MAIN(TestAssetManager)
#include "test_assetmanager.moc"
