#pragma once
#include <QObject>
#include <QString>

class ConfigManager : public QObject {
    Q_OBJECT
public:
    enum QualityTier { Low = 0, Medium = 1, HD = 2 };
    Q_ENUM(QualityTier)

    explicit ConfigManager(const QString& bodyId = QStringLiteral("earth"),
                           QObject *parent = nullptr);

    // Derives the config file path for a body. Earth keeps the legacy
    // top-level config.json (Phase 1 compat) so existing user settings survive;
    // other bodies get a per-body subdir. Pure function — no filesystem access.
    static QString pathForBody(const QString& bodyId);

    // Resolved config file path (for diagnostics/testing).
    QString path() const { return m_path; }

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
    bool alwaysOnTop() const;     void setAlwaysOnTop(bool v);

    void load();
    void save();

private:
    // Test-only constructor: uses an explicit directory (appends "/config.json")
    // without QStandardPaths derivation or directory creation, so load/save
    // round-trip tests stay hermetic against a QTemporaryDir.
    ConfigManager(const QString& dir, bool /*testing*/, QObject *parent = nullptr);
    friend class TestConfigManager;

    QString m_path;
    int m_x = 100, m_y = 100, m_diameter = 220, m_fpsCap = 60;
    int m_rotationSpeed = 2880;  // globe-toy spin: sun fixed at real position, Earth spins under it (2880x = ~1 turn/30s)
    QualityTier m_tier = HD;
    double m_cloudSpeed = 1.0;
    bool m_locationOptIn = false;
    bool m_showGrid = false;
    bool m_alwaysOnTop = true;
    QString m_nightMode = QStringLiteral("texture");
    QString m_language  = QStringLiteral("en");
    double m_homeLat = 0.0, m_homeLon = 0.0;
    static int clampDiameter(int v);
};
