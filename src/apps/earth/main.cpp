#include "CelestialWidget.h"
#include "ConfigManager.h"
#include "SystemTray.h"
#include "SettingsDialog.h"
#include "FullscreenWatcher.h"
#include "SunModel.h"
#include "CameraController.h"
#include "AssetManager.h"
#include "TimeController.h"
#include "LocationProvider.h"
#include "BodyConfig.h"
#include <QApplication>
#include <QCoreApplication>
#include <QTimer>
#include <QDateTime>
#include <QTranslator>
#include <QSharedMemory>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

int main(int argc, char *argv[]) {
#ifdef Q_OS_WIN
    {
        HANDLE mutex = CreateMutexW(nullptr, TRUE, L"GlobeAppSingleton_Earth_v1");
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            if (mutex) CloseHandle(mutex);
            return 0;
        }
    }
#else
    {
        QSharedMemory shm(QStringLiteral("GlobeAppSingleton_Earth_v1"));
        if (!shm.create(1)) return 0;
    }
#endif

    QApplication app(argc, argv);
    // Phase 1 keeps the legacy "Globe" application name so that
    // QStandardPaths::AppConfigLocation resolves to the same directory the old
    // globe.exe used, preserving the user's existing config.json. Per-body
    // config directories will be introduced in Phase 2.
    app.setApplicationName("Globe");

    ConfigManager config;

    QTranslator translator;
    if (config.language() == QStringLiteral("zh_CN")) {
        if (translator.load(QStringLiteral(":/i18n/globe_zh_CN.qm")))
            app.installTranslator(&translator);
    }

    AssetManager assets;
    SunModel sun;
    CameraController camera;

    BodyConfig earthConfig = BodyConfig::earth();
    CelestialWidget widget(earthConfig, &config);
    widget.setCameraController(&camera);
    widget.body().setAssets(&assets);
    widget.body().setSun(&sun);
    {
        const int tierMax = (config.qualityTier() == ConfigManager::HD) ? 8192 :
                            (config.qualityTier() == ConfigManager::Medium) ? 4096 : 2048;
        widget.body().setQualityTier(tierMax);
    }

    widget.move(config.windowX(), config.windowY());
    widget.resize(config.diameter(), config.diameter());

    LocationProvider location;
    if (config.locationOptIn()) location.start();
    widget.applyOptionsFromConfig();  // sets home location, grid, night mode, rotation speed

    QObject::connect(&location, &LocationProvider::locationChanged, &widget,
        [&widget, &config](double lat, double lon) {
            // Persist the fix so it survives restart, then let the widget
            // update its beacon (gated by opt-in inside the widget).
            config.setHomeLatitude(lat);
            config.setHomeLongitude(lon);
            config.save();
            widget.setHomeLocation(lat, lon);
        });

    TimeController time(&sun);
    time.setTarget(&widget);
    time.setFpsCap(config.fpsCap());
    // Real-time spin (rotationSpeed == 1) barely moves per frame, so idle at
    // the low baseline; faster spins need the full fps cap to look smooth.
    time.setAnimating(config.rotationSpeed() > 1);
    // User drag/wheel snaps the repaint rate back up to the cap for ~2s.
    QObject::connect(&widget, &CelestialWidget::userInteracted, &time, [&time] { time.boost(); });

    SystemTray tray;
    SettingsDialog settingsDialog(&config, &widget);
    QObject::connect(&tray, &SystemTray::toggleVisibility, &widget,
        [&widget] { widget.isVisible() ? widget.hide() : widget.show(); });
    QObject::connect(&settingsDialog, &SettingsDialog::settingsChanged, &widget,
        [&widget, &config, &translator, &app, &time]() {
            // applyOptionsFromConfig() toggles Qt::WindowStaysOnTopHint; changing
            // window flags hides a visible widget, so capture and re-show it.
            const bool wasVisible = widget.isVisible();
            widget.applyOptionsFromConfig();
            if (wasVisible) widget.show();
            // Spin speed may have changed: re-evaluate the adaptive baseline.
            time.setAnimating(config.rotationSpeed() > 1);
            app.removeTranslator(&translator);
            if (config.language() == QStringLiteral("zh_CN")) {
                if (translator.load(QStringLiteral(":/i18n/globe_zh_CN.qm")))
                    app.installTranslator(&translator);
            }
            widget.update();
        });
    QObject::connect(&tray, &SystemTray::openSettings, &widget,
        [&settingsDialog]() { settingsDialog.exec(); });
    QObject::connect(&tray, &SystemTray::resetView, &widget, [&widget, &camera] {
        camera.reset();
        CelestialRenderOptions opts = widget.body().options();
        opts.useCenterLon = false;
        widget.body().setOptions(opts);
        widget.update();
    });
    QObject::connect(&tray, &SystemTray::centerOnMe, &widget, [&widget, &camera, &config] {
        camera.reset();
        CelestialRenderOptions opts = widget.body().options();
        opts.centerLon = config.homeLongitude();
        opts.useCenterLon = true;
        widget.body().setOptions(opts);
        widget.update();
    });

    auto refreshTooltip = [&sun, &tray] {
        sun.setUtc(QDateTime::currentDateTimeUtc());
        tray.setSolarTooltip(QCoreApplication::translate("CelestialWidget", "Sun sub-point: %1°, %2°")
            .arg(sun.subSolarLatitude(), 0, 'f', 1)
            .arg(sun.subSolarLongitude(), 0, 'f', 1));
    };
    refreshTooltip();
    QTimer tooltipTimer;
    QObject::connect(&tooltipTimer, &QTimer::timeout, &widget, refreshTooltip);
    tooltipTimer.start(300000);

    FullscreenWatcher fullscreenWatcher;
    QObject::connect(
        &fullscreenWatcher, &FullscreenWatcher::fullscreenAppStarted, &widget,
        [&widget] { widget.hide(); });
    QObject::connect(
        &fullscreenWatcher, &FullscreenWatcher::fullscreenAppStopped, &widget,
        [&widget] { widget.show(); });
    fullscreenWatcher.start();

    widget.show();
    return app.exec();
}
