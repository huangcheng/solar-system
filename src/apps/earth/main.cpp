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

    ConfigManager config(QStringLiteral("earth"));  // explicit bodyId; preserves legacy config.json path

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
        [&widget](double lat, double lon) {
            CelestialRenderOptions opts = widget.body().options();
            opts.homeLat = lat; opts.homeLon = lon; opts.hasHome = true;
            widget.body().setOptions(opts);
            widget.update();
        });

    TimeController time(&sun);
    time.setTarget(&widget);
    time.setFpsCap(config.fpsCap());

    SystemTray tray;
    SettingsDialog settingsDialog(&config, &widget);
    QObject::connect(&tray, &SystemTray::toggleVisibility, &widget,
        [&widget] { widget.isVisible() ? widget.hide() : widget.show(); });
    QObject::connect(&settingsDialog, &SettingsDialog::settingsChanged, &widget,
        [&widget, &config, &translator, &app]() {
            // applyOptionsFromConfig() toggles Qt::WindowStaysOnTopHint; changing
            // window flags hides a visible widget, so capture and re-show it.
            const bool wasVisible = widget.isVisible();
            widget.applyOptionsFromConfig();
            if (wasVisible) widget.show();
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
        widget.resize(220, 220);
        widget.update();
    });
    QObject::connect(&tray, &SystemTray::centerOnMe, &widget, [&widget, &camera, &config] {
        camera.reset();
        CelestialRenderOptions opts = widget.body().options();
        opts.centerLon = config.homeLongitude();
        opts.useCenterLon = true;
        widget.body().setOptions(opts);
        widget.resize(220, 220);
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
