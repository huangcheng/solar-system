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
    QString nightMode() const;    void setNightMode(const QString &v);
    QString language() const;     void setLanguage(const QString &v);

    void load();
    void save();

private:
    QString m_path;
    int m_x = 100, m_y = 100, m_diameter = 220, m_fpsCap = 60;
    int m_rotationSpeed = 1;     // 1 = static (real-time day/night); >1 adds a visible spin
    QualityTier m_tier = HD;
    double m_cloudSpeed = 1.0;
    bool m_locationOptIn = false;
    bool m_showGrid = false;
    QString m_nightMode = QStringLiteral("simple");
    QString m_language  = QStringLiteral("en");
    double m_homeLat = 0.0, m_homeLon = 0.0;
    static int clampDiameter(int v);
};
