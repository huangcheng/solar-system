#include "ConfigManager.h"
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QSettings>
#include <QDir>
#include <QTextStream>

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
double ConfigManager::homeLatitude() const { return m_homeLat; }
void ConfigManager::setHomeLatitude(double v) { m_homeLat = v; }
bool ConfigManager::showGrid() const { return m_showGrid; }
void ConfigManager::setShowGrid(bool v) { m_showGrid = v; }
bool ConfigManager::showHomeMarker() const { return m_showHomeMarker; }
void ConfigManager::setShowHomeMarker(bool v) { m_showHomeMarker = v; }
QString ConfigManager::nightMode() const { return m_nightMode; }
void ConfigManager::setNightMode(const QString &v) { m_nightMode = (v == QStringLiteral("texture") ? v : QStringLiteral("simple")); }
QString ConfigManager::language() const { return m_language; }
void ConfigManager::setLanguage(const QString &v) { m_language = (v == QStringLiteral("zh_CN") ? v : QStringLiteral("en")); }
bool ConfigManager::alwaysOnTop() const { return m_alwaysOnTop; }
void ConfigManager::setAlwaysOnTop(bool v) { m_alwaysOnTop = v; }
QString ConfigManager::viewMode() const { return m_viewMode; }
void ConfigManager::setViewMode(const QString &v) {
    m_viewMode = (v == QStringLiteral("map")) ? QStringLiteral("map") : QStringLiteral("globe");
}
bool ConfigManager::autoDetectOnStart() const { return m_autoDetectOnStart; }
void ConfigManager::setAutoDetectOnStart(bool v) { m_autoDetectOnStart = v; }
bool ConfigManager::launchOnLogin() const { return m_launchOnLogin; }
void ConfigManager::setLaunchOnLogin(bool v) { m_launchOnLogin = v; }

void ConfigManager::applyLaunchOnLogin(bool enable) {
    QString exe = QCoreApplication::applicationFilePath();
#if defined(Q_OS_WIN)
    // Per-user Run key (no elevation needed). Native separators + quotes so the
    // shell launches it verbatim at login even if the path contains spaces.
    exe = QDir::toNativeSeparators(exe);
    QSettings run(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                  QSettings::NativeFormat);
    if (enable)
        run.setValue(QStringLiteral("Globe"), QStringLiteral("\"%1\"").arg(exe));
    else
        run.remove(QStringLiteral("Globe"));
#elif defined(Q_OS_MACOS)
    const QString dir = QDir::homePath() + QStringLiteral("/Library/LaunchAgents");
    const QString path = dir + QStringLiteral("/com.huangcheng.globe.plist");
    if (enable) {
        QDir().mkpath(dir);
        QFile f(path);
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QTextStream s(&f);
            s << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
              << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
              << "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
              << "<plist version=\"1.0\">\n<dict>\n"
              << "  <key>Label</key><string>com.huangcheng.globe</string>\n"
              << "  <key>ProgramArguments</key>\n  <array>\n"
              << "    <string>" << exe << "</string>\n"
              << "  </array>\n"
              << "  <key>RunAtLoad</key><true/>\n"
              << "</dict>\n</plist>\n";
        }
    } else {
        QFile::remove(path);
    }
#else
    // XDG autostart on Linux/*BSD.
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                        + QStringLiteral("/autostart");
    const QString path = dir + QStringLiteral("/globe.desktop");
    if (enable) {
        QDir().mkpath(dir);
        QFile f(path);
        if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QTextStream s(&f);
            s << "[Desktop Entry]\n"
              << "Type=Application\n"
              << "Name=Globe\n"
              << "Exec=" << exe << "\n"
              << "Terminal=false\n"
              << "X-GNOME-Autostart-enabled=true\n";
        }
    } else {
        QFile::remove(path);
    }
#endif
}

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
    m_showHomeMarker = obj.value("showHomeMarker").toBool(m_showHomeMarker);
    m_nightMode = obj.value("nightMode").toString(m_nightMode) == QStringLiteral("texture")
                      ? QStringLiteral("texture") : QStringLiteral("simple");
    m_language  = obj.value("language").toString(m_language) == QStringLiteral("zh_CN")
                      ? QStringLiteral("zh_CN") : QStringLiteral("en");
    m_alwaysOnTop = obj.value("alwaysOnTop").toBool(m_alwaysOnTop);
    m_autoDetectOnStart = obj.value("autoDetectOnStart").toBool(m_autoDetectOnStart);
    m_launchOnLogin = obj.value("launchOnLogin").toBool(m_launchOnLogin);
    m_viewMode = (obj.value("viewMode").toString(m_viewMode) == QStringLiteral("map"))
                     ? QStringLiteral("map") : QStringLiteral("globe");
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
    o["showHomeMarker"] = m_showHomeMarker;
    o["nightMode"] = m_nightMode;
    o["language"]  = m_language;
    o["alwaysOnTop"] = m_alwaysOnTop;
    o["autoDetectOnStart"] = m_autoDetectOnStart;
    o["launchOnLogin"] = m_launchOnLogin;
    o["viewMode"] = m_viewMode;
    QFile f(m_path);
    // AppConfigLocation is not guaranteed to exist yet; without this, QFile
    // silently fails to open for writing and NOTHING persists across launches.
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    if (f.open(QIODevice::WriteOnly))
        f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
}
