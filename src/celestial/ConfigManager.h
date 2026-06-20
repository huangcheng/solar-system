#pragma once
#include <QObject>
#include <QString>

class ConfigManager : public QObject {
    Q_OBJECT
public:
    enum QualityTier { Low = 0, Medium = 1, HD = 2 };
    Q_ENUM(QualityTier)

    explicit ConfigManager(QString dir = QString(), QObject *parent = nullptr);

    int windowX() const;        void setWindowX(int v);
    int windowY() const;        void setWindowY(int v);
    int diameter() const;       void setDiameter(int v);
    QualityTier qualityTier() const; void setQualityTier(QualityTier t);
    int fpsCap() const;         void setFpsCap(int v);
    double cloudSpeed() const;  void setCloudSpeed(double v);
    int rotationSpeed() const;  void setRotationSpeed(int v); // ratio over real-time (1 = true 24h)
    bool locationOptIn() const; void setLocationOptIn(bool v);
    double homeLatitude() const;  void setHomeLatitude(double v);
    double homeLongitude() const; void setHomeLongitude(double v);
    bool showGrid() const;        void setShowGrid(bool v);
    bool showHomeMarker() const;  void setShowHomeMarker(bool v);
    QString nightMode() const;    void setNightMode(const QString &v);
    QString language() const;     void setLanguage(const QString &v);
    bool alwaysOnTop() const;     void setAlwaysOnTop(bool v);
    QString viewMode() const;     void setViewMode(const QString &v);
    bool autoDetectOnStart() const; void setAutoDetectOnStart(bool v);

    void load();
    void save();

private:
    QString m_path;
    int m_x = 100, m_y = 100, m_diameter = 220, m_fpsCap = 60;
    int m_rotationSpeed = 2880;  // globe-toy spin: sun fixed at real position, Earth spins under it (2880x = ~1 turn/30s)
    QualityTier m_tier = HD;
    double m_cloudSpeed = 1.0;
    bool m_locationOptIn = false;
    bool m_showGrid = false;
    bool m_showHomeMarker = false;
    bool m_alwaysOnTop = true;
    QString m_nightMode = QStringLiteral("texture");
    QString m_language  = QStringLiteral("en");
    QString m_viewMode  = QStringLiteral("globe");  // "globe" (3D) or "map" (flat equirectangular)
    double m_homeLat = 0.0, m_homeLon = 0.0;
    bool m_autoDetectOnStart = false;
    static int clampDiameter(int v);
};
