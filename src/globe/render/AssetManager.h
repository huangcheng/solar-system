#pragma once
#include <QImage>
#include <QString>
#include <QStringList>

class ConfigManager;

// Loads day/night/cloud textures from disk (never from the Qt resource system
// — HD images stay as loose files to keep the binary small). It searches a list
// of candidate directories in order and uses the first file it finds; missing
// files fall back to procedural images so the widget always renders.
class AssetManager {
public:
    enum Slot { Day = 0, Night = 1, Clouds = 2 };

    explicit AssetManager(QString dir = QString());

    QImage image(Slot slot, int tierMaxSize) const;
    bool hasFile(Slot slot) const;

private:
    QStringList m_dirs;                 // searched in order
    QString pathFor(Slot slot) const;   // first existing file across m_dirs
    static QString fileName(Slot slot);
    static QImage fallback(Slot slot);
};
