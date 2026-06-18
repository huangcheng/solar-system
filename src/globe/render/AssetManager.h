#pragma once
#include <QImage>
#include <QString>

class ConfigManager;

// Loads day/night/cloud textures from disk, downscaling to the active quality
// tier. Missing files are replaced with procedural fallbacks so the widget
// always renders. Only produces QImages; the renderer uploads to GL.
class AssetManager {
public:
    enum Slot { Day = 0, Night = 1, Clouds = 2 };

    explicit AssetManager(QString dir = QString());

    QImage image(Slot slot, int tierMaxSize) const;
    bool hasFile(Slot slot) const;

private:
    QString m_dir;
    QString pathFor(Slot slot) const;
    static QImage fallback(Slot slot);
};
