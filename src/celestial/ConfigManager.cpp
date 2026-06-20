#include "ConfigManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>

int ConfigManager::clampDiameter(int v) { return v < 160 ? 160 : (v > 1024 ? 1024 : v); }

QString ConfigManager::pathForBody(const QString& bodyId) {
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    // Phase 1 compat: earth keeps the legacy top-level config.json so the user's
    // existing settings survive. Other bodies get a per-body subdir.
    return (bodyId == QStringLiteral("earth")) ? (base + "/config.json")
                                               : (base + "/" + bodyId + "/config.json");
}

ConfigManager::ConfigManager(const QString& bodyId, const QString& baseDir, QObject *parent)
    : QObject(parent) {
    if (baseDir.isEmpty()) {
        m_path = pathForBody(bodyId);
    } else {
        // Explicit baseDir override (portable installs, --config-dir, hermetic
        // tests). Mirrors pathForBody's earth rule: earth stays at the top
        // level (<baseDir>/config.json), other bodies get a per-body subdir.
        const QString mid = (bodyId == QStringLiteral("earth")) ? QString() : bodyId + "/";
        m_path = baseDir + "/" + mid + "config.json";
    }
    load();
}

int ConfigManager::windowX() const { return m_x; }
void ConfigManager::setWindowX(int v) { m_x = v; }
int ConfigManager::windowY() const { return m_y; }
void ConfigManager::setWindowY(int v) { m_y = v; }
int ConfigManager::diameter() const { return m_diameter; }
void ConfigManager::setDiameter(int v) { m_diameter = clampDiameter(v); }
ConfigManager::QualityTier ConfigManager::qualityTier() const { return m_tier; }
void ConfigManager::setQualityTier(QualityTier t) { m_tier = t; }
int ConfigManager::fpsCap() const { return m_fpsCap; }
void ConfigManager::setFpsCap(int v) { m_fpsCap = (v == 30 ? 30 : 60); }
double ConfigManager::cloudSpeed() const { return m_cloudSpeed; }
void ConfigManager::setCloudSpeed(double v) { m_cloudSpeed = v; }
int ConfigManager::rotationSpeed() const { return m_rotationSpeed; }
void ConfigManager::setRotationSpeed(int v) { m_rotationSpeed = (v < 1 ? 1 : v); }
bool ConfigManager::locationOptIn() const { return m_locationOptIn; }
void ConfigManager::setLocationOptIn(bool v) { m_locationOptIn = v; }
double ConfigManager::homeLongitude() const { return m_homeLon; }
void ConfigManager::setHomeLongitude(double v) { m_homeLon = v; }
double ConfigManager::homeLatitude() const { return m_homeLat; }
void ConfigManager::setHomeLatitude(double v) { m_homeLat = v; }
bool ConfigManager::showGrid() const { return m_showGrid; }
void ConfigManager::setShowGrid(bool v) { m_showGrid = v; }
QString ConfigManager::nightMode() const { return m_nightMode; }
void ConfigManager::setNightMode(const QString &v) { m_nightMode = (v == QStringLiteral("texture") ? v : QStringLiteral("simple")); }
QString ConfigManager::language() const { return m_language; }
void ConfigManager::setLanguage(const QString &v) { m_language = (v == QStringLiteral("zh_CN") ? v : QStringLiteral("en")); }
bool ConfigManager::alwaysOnTop() const { return m_alwaysOnTop; }
void ConfigManager::setAlwaysOnTop(bool v) { m_alwaysOnTop = v; }

void ConfigManager::load() {
    QFile f(m_path);
    if (!f.open(QIODevice::ReadOnly)) return;
    const auto obj = QJsonDocument::fromJson(f.readAll()).object();
    m_x = obj.value("windowX").toInt(m_x);
    m_y = obj.value("windowY").toInt(m_y);
    m_diameter = clampDiameter(obj.value("diameter").toInt(m_diameter));
    m_tier = static_cast<QualityTier>(obj.value("quality").toInt(static_cast<int>(m_tier)));
    m_fpsCap = (obj.value("fpsCap").toInt(m_fpsCap) == 30 ? 30 : 60);
    m_cloudSpeed = obj.value("cloudSpeed").toDouble(m_cloudSpeed);
    { const int rs = obj.value("rotationSpeed").toInt(m_rotationSpeed); m_rotationSpeed = (rs < 1 ? 1 : rs); }
    m_locationOptIn = obj.value("locationOptIn").toBool(m_locationOptIn);
    m_homeLat = obj.value("homeLatitude").toDouble(m_homeLat);
    m_homeLon = obj.value("homeLongitude").toDouble(m_homeLon);
    m_showGrid = obj.value("showGrid").toBool(m_showGrid);
    m_nightMode = obj.value("nightMode").toString(m_nightMode) == QStringLiteral("texture")
                      ? QStringLiteral("texture") : QStringLiteral("simple");
    m_language  = obj.value("language").toString(m_language) == QStringLiteral("zh_CN")
                      ? QStringLiteral("zh_CN") : QStringLiteral("en");
    m_alwaysOnTop = obj.value("alwaysOnTop").toBool(m_alwaysOnTop);
}

void ConfigManager::save() {
    QJsonObject o;
    o["windowX"] = m_x; o["windowY"] = m_y; o["diameter"] = m_diameter;
    o["quality"] = static_cast<int>(m_tier); o["fpsCap"] = m_fpsCap;
    o["cloudSpeed"] = m_cloudSpeed; o["locationOptIn"] = m_locationOptIn;
    o["rotationSpeed"] = m_rotationSpeed;
    o["homeLatitude"] = m_homeLat;
    o["homeLongitude"] = m_homeLon;
    o["showGrid"] = m_showGrid;
    o["nightMode"] = m_nightMode;
    o["language"]  = m_language;
    o["alwaysOnTop"] = m_alwaysOnTop;
    // Ensure the parent dir exists before writing (read-only profiles can construct freely).
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    QFile f(m_path);
    if (f.open(QIODevice::WriteOnly))
        f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
}
