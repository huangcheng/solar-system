#include "AssetManager.h"
#include <QFile>
#include <QStandardPaths>
#include <QCoreApplication>

AssetManager::AssetManager(QString dir) {
    // Search order: explicit dir, then the folder next to the binary (where the
    // installer ships HD textures), then the dev tree (../textures when running
    // from the build dir), then the user-local cache.
    if (!dir.isEmpty()) m_dirs.append(dir);
    m_dirs.append(QCoreApplication::applicationDirPath() + "/textures");
    m_dirs.append(QCoreApplication::applicationDirPath() + "/../textures");
    m_dirs.append(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/textures");
}

QString AssetManager::fileName(Slot slot) {
    switch (slot) {
    case Day:      return QStringLiteral("day.jpg");
    case Night:    return QStringLiteral("night.jpg");
    case Clouds:   return QStringLiteral("clouds.png");
    case Normal:   return QStringLiteral("normal.png");
    case Specular: return QStringLiteral("specular.png");
    }
    return {};
}

QString AssetManager::pathFor(Slot slot) const {
    const QString name = fileName(slot);
    for (const QString &d : m_dirs) {
        const QString p = d + QLatin1Char('/') + name;
        if (QFile::exists(p)) return p;
    }
    // Nothing found: return a (nonexistent) path; the loader falls back to a
    // procedural image.
    return m_dirs.constFirst() + QLatin1Char('/') + name;
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
