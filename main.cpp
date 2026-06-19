#include "globe/GlobeWindow.h"
#include "globe/ConfigManager.h"
#include "globe/SystemTray.h"
#include "globe/SettingsDialog.h"
#include "globe/FullscreenWatcher.h"
#include "render/SunModel.h"
#include "render/CameraController.h"
#include "render/AssetManager.h"
#include "render/TimeController.h"
#include "globe/LocationProvider.h"
#include <QApplication>
#include <QStandardPaths>
#include <QTimer>
#include <QDebug>
#include <QDateTime>
#include <QTranslator>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Globe");

    QTranslator translator;
    {
        ConfigManager tmpConfig;
        const QString lang = tmpConfig.language();
        if (lang == QStringLiteral("zh_CN")) {
            if (translator.load(QStringLiteral(":/i18n/globe_zh_CN.qm")))
                app.installTranslator(&translator);
        }
    }

    ConfigManager config;
    AssetManager assets;  // loads from <exedir>/textures (shipped), then user-local, then procedural fallback
    SunModel sun;
    CameraController camera;

    GlobeWindow w;
    w.view()->renderer().setAssets(&assets);
    w.view()->renderer().setSun(&sun);
    w.view()->renderer().setCamera(&camera);
    w.view()->renderer().setRotationSpeedRatio(config.rotationSpeed());
    {
        const int tierMax = (config.qualityTier() == ConfigManager::HD) ? 8192 :
                            (config.qualityTier() == ConfigManager::Medium) ? 4096 : 2048;
        w.view()->renderer().setQualityTier(tierMax);
    }
    w.setCameraController(&camera);
    w.move(config.windowX(), config.windowY());
    w.resize(config.diameter(), config.diameter());

    // Location: opt-in OS geolocation (Qt Positioning). The pulsing beacon shows
    // at the configured home by default; if the user opts in and a valid fix
    // arrives, it overrides with the real coordinates.
    LocationProvider location;
    if (config.locationOptIn()) location.start();
    w.view()->renderer().setHomeLocation(config.homeLatitude(), config.homeLongitude(), true);
    w.view()->renderer().setShowGrid(config.showGrid());
    w.view()->renderer().setUseNightTexture(config.nightMode() == QStringLiteral("texture"));
    QObject::connect(&location, &LocationProvider::locationChanged, &w,
        [&w](double lat, double lon) { w.view()->renderer().setHomeLocation(lat, lon, true); });

    TimeController time(&sun);
    time.setTarget(w.view());
    time.setFpsCap(config.fpsCap());

    w.setConfig(&config);

    SystemTray tray;
    SettingsDialog settingsDialog(&config, &w);
    QObject::connect(&tray, &SystemTray::toggleVisibility, &w,
        [&w] { w.isVisible() ? w.hide() : w.show(); });
    QObject::connect(&settingsDialog, &SettingsDialog::settingsChanged, &w,
        [&w, &config, &translator, &app]() {
            const bool useTexture = config.nightMode() == QStringLiteral("texture");
            w.view()->renderer().setShowGrid(config.showGrid());
            w.view()->renderer().setUseNightTexture(useTexture);
            app.removeTranslator(&translator);
            if (config.language() == QStringLiteral("zh_CN")) {
                if (translator.load(QStringLiteral(":/i18n/globe_zh_CN.qm")))
                    app.installTranslator(&translator);
            }
            w.view()->update();
        });
    QObject::connect(&tray, &SystemTray::openSettings, &w,
        [&settingsDialog]() { settingsDialog.exec(); });
    QObject::connect(&tray, &SystemTray::resetView, &w, [&w, &camera] {
        camera.reset();
        w.view()->renderer().clearCenterLongitude();
        w.resize(220, 220);
        w.view()->update();
    });
    QObject::connect(&tray, &SystemTray::centerOnMe, &w, [&w, &camera, &config] {
        camera.reset();
        w.view()->renderer().setCenterLongitude(config.homeLongitude());
        w.resize(220, 220);
        w.view()->update();
    });

    // Live solar tooltip: refresh the tray with the current sun sub-point.
    auto refreshTooltip = [&sun, &tray] {
        sun.setUtc(QDateTime::currentDateTimeUtc());
        tray.setSolarTooltip(GlobeWindow::tr("Sun sub-point: %1°, %2°")
            .arg(sun.subSolarLatitude(), 0, 'f', 1)
            .arg(sun.subSolarLongitude(), 0, 'f', 1));
    };
    refreshTooltip();
    QTimer tooltipTimer;
    QObject::connect(&tooltipTimer, &QTimer::timeout, &w, refreshTooltip);
    tooltipTimer.start(300000); // every 5 min

    // Hide the globe when a fullscreen app (game, video player, etc.) is in the
    // foreground, so it doesn't float on top of fullscreen content.
    FullscreenWatcher fullscreenWatcher;
    QObject::connect(&fullscreenWatcher, &FullscreenWatcher::fullscreenAppStarted, &w,
        [&w] { w.hide(); });
    QObject::connect(&fullscreenWatcher, &FullscreenWatcher::fullscreenAppStopped, &w,
        [&w] { w.show(); });
    fullscreenWatcher.start();

    w.show();
    return app.exec();
}
