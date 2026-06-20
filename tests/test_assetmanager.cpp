#include <QtTest/QtTest>
#include "celestial/AssetManager.h"

class TestAssetManager : public QObject {
    Q_OBJECT
private slots:
    void filenameLookupResolvesExistingFile();
    void filenameLookupFallsBackWhenMissing();
};

void TestAssetManager::filenameLookupResolvesExistingFile() {
    // earth day.jpg ships in the repo's textures/ dir (../textures from build dir)
    AssetManager am;
    QImage img = am.image(QStringLiteral("day.jpg"), 4096);
    QVERIFY(!img.isNull());
    QVERIFY(img.width() > 0);
}

void TestAssetManager::filenameLookupFallsBackWhenMissing() {
    AssetManager am;
    QImage img = am.image(QStringLiteral("does_not_exist_xyz.jpg"), 4096);
    QVERIFY(!img.isNull());  // procedural fallback, not null
}

QTEST_MAIN(TestAssetManager)
#include "test_assetmanager.moc"
