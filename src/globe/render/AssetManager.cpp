#include "AssetManager.h"
#include <QFile>
#include <QStandardPaths>

AssetManager::AssetManager(QString dir) {
    if (dir.isEmpty())
        dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/textures";
    m_dir = dir;
}

QString AssetManager::pathFor(Slot slot) const {
    switch (slot) {
    case Day:    return m_dir + "/day.jpg";
    case Night:  return m_dir + "/night.jpg";
    case Clouds: return m_dir + "/clouds.png";
    }
    return {};
}

bool AssetManager::hasFile(Slot slot) const {
    return QFile::exists(pathFor(slot));
}

QImage AssetManager::fallback(Slot slot) {
    QImage img(512, 256, QImage::Format_RGB32);
    switch (slot) {
    case Day:    img.fill(QColor(30, 70, 150));  break;  // ocean blue
    case Night:  img.fill(QColor(0, 0, 0));      break;
    case Clouds: img.fill(QColor(0, 0, 0));      break;  // no clouds
    }
    return img;
}

QImage AssetManager::image(Slot slot, int tierMaxSize) const {
    QImage img = hasFile(slot) ? QImage(pathFor(slot)) : fallback(slot);
    if (img.isNull()) img = fallback(slot);
    const int w = img.width(), h = img.height();
    if (w > tierMaxSize) {
        const int nh = int(qint64(h) * tierMaxSize / qMax(1, w));
        img = img.scaled(tierMaxSize, nh, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    return img;
}
