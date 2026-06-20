#pragma once
#include <QImage>
#include <QString>
#include <QStringList>

// Loads day/night/cloud textures from disk (never from the Qt resource system
// — HD images stay as loose files to keep the binary small). It searches a list
// of candidate directories in order and uses the first file it finds; missing
// files fall back to procedural images so the widget always renders.
class AssetManager {
public:
    enum Slot { Day = 0, Night = 1, Clouds = 2, Normal = 3, Specular = 4 };

    explicit AssetManager(QString dir = QString());

    QImage image(Slot slot, int tierMaxSize) const;
    bool hasFile(Slot slot) const;

    // Body-agnostic API: load any named texture from the search dirs (used by
    // BodyConfig-driven bodies like Sun/Moon/planets). Returns a procedural
    // fallback (solid neutral gray) if not found, never a null QImage.
    // tierMaxSize caps the long edge.
    QImage image(const QString& fileName, int tierMaxSize) const;
    bool hasFile(const QString& fileName) const;

private:
    QStringList m_dirs;                 // searched in order
    QString pathFor(Slot slot) const;   // first existing file across m_dirs
    static QString fileName(Slot slot);
    static QImage fallback(Slot slot);
    static QImage scaledToTier(QImage img, int tierMaxSize);
};
