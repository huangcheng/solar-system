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
    bool locationOptIn() const; void setLocationOptIn(bool v);
    double homeLongitude() const; void setHomeLongitude(double v);

    void load();
    void save();

private:
    QString m_path;
    int m_x = 100, m_y = 100, m_diameter = 360, m_fpsCap = 60;
    QualityTier m_tier = HD;
    double m_cloudSpeed = 1.0;
    bool m_locationOptIn = false;
    double m_homeLon = 0.0;
    static int clampDiameter(int v);
};
