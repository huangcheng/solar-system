#include "ConfigManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

int ConfigManager::clampDiameter(int v) { return v < 160 ? 160 : (v > 1024 ? 1024 : v); }

ConfigManager::ConfigManager(QString dir, QObject *parent) : QObject(parent) {
    if (dir.isEmpty())
        dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    m_path = dir + "/config.json";
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
    m_homeLon = obj.value("homeLongitude").toDouble(m_homeLon);
}

void ConfigManager::save() {
    QJsonObject o;
    o["windowX"] = m_x; o["windowY"] = m_y; o["diameter"] = m_diameter;
    o["quality"] = static_cast<int>(m_tier); o["fpsCap"] = m_fpsCap;
    o["cloudSpeed"] = m_cloudSpeed; o["locationOptIn"] = m_locationOptIn;
    o["rotationSpeed"] = m_rotationSpeed;
    o["homeLongitude"] = m_homeLon;
    QFile f(m_path);
    if (f.open(QIODevice::WriteOnly))
        f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
}
