#include "BodyApp.h"
#include "BodyConfig.h"
#include "CelestialWidget.h"
#include "CelestialBody.h"
#include "ConfigManager.h"
#include "SystemTray.h"
#include "SettingsDialog.h"
#include "FullscreenWatcher.h"
#include "SunModel.h"
#include "CameraController.h"
#include "AssetManager.h"
#include "TimeController.h"
#include <QApplication>
#include <QTranslator>
#include <QSharedMemory>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace bodyapp {

static QString singletonKey(const QString& bodyId) {
    return QStringLiteral("GlobeAppSingleton_") + bodyId + QStringLiteral("_v1");
}

int runBodyApp(int argc, char *argv[], const BodyConfig& config) {
    // Singleton guard: only one instance of each body runs at a time. Keyed off
    // config.bodyId so sun/moon/each planet get independent singletons.
    const QString key = singletonKey(config.bodyId);
#ifdef Q_OS_WIN
    {
        const QString mutexName = key;  // same string, wide via utf16()
        HANDLE m = CreateMutexW(nullptr, TRUE,
            reinterpret_cast<LPCWSTR>(mutexName.utf16()));
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            if (m) CloseHandle(m);
            return 0;
        }
    }
#else
    {
        QSharedMemory shm(key);
        if (!shm.create(1)) return 0;
    }
#endif

    QApplication app(argc, argv);
    // Shared "Globe" application name so QStandardPaths::AppConfigLocation is
    // common across the family; ConfigManager scopes per-body via bodyId.
    app.setApplicationName(QStringLiteral("Globe"));

    ConfigManager cm(config.bodyId);

    QTranslator translator;
    if (cm.language() == QStringLiteral("zh_CN")) {
        if (translator.load(QStringLiteral(":/i18n/globe_zh_CN.qm")))
            app.installTranslator(&translator);
    }

    AssetManager assets;
    SunModel sun;            // drives day/night shading; harmless for emissive Sun
    CameraController camera;

    CelestialWidget widget(config, &cm);
    widget.setCameraController(&camera);
    widget.body().setAssets(&assets);
    widget.body().setSun(&sun);
    {
        const int tierMax = (cm.qualityTier() == ConfigManager::HD) ? 8192 :
                            (cm.qualityTier() == ConfigManager::Medium) ? 4096 : 2048;
        widget.body().setQualityTier(tierMax);
    }
    widget.move(cm.windowX(), cm.windowY());
    widget.resize(cm.diameter(), cm.diameter());
    widget.applyOptionsFromConfig();
    // No "you are here" marker on non-Earth bodies — the home beacon is
    // meaningless on alien worlds. (applyOptionsFromConfig sets hasHome=true
    // unconditionally because Earth always shows its configured home.)
    {
        CelestialRenderOptions opts = widget.body().options();
        opts.hasHome = false;
        widget.body().setOptions(opts);
    }

    TimeController time(&sun);
    time.setTarget(&widget);
    time.setFpsCap(cm.fpsCap());

    SystemTray tray(false);  // no "Center on Me" — body apps don't wire it up
    tray.setSolarTooltip(config.displayName);  // static body-name tooltip
    SettingsDialog settingsDialog(&cm, &widget);
    QObject::connect(&tray, &SystemTray::toggleVisibility, &widget,
        [&widget] { widget.isVisible() ? widget.hide() : widget.show(); });
    QObject::connect(&settingsDialog, &SettingsDialog::settingsChanged, &widget,
        [&widget, &cm, &translator, &app]() {
            // applyOptionsFromConfig() toggles Qt::WindowStaysOnTopHint; changing
            // window flags hides a visible widget, so capture and re-show it.
            const bool wasVisible = widget.isVisible();
            widget.applyOptionsFromConfig();
            if (wasVisible) widget.show();
            app.removeTranslator(&translator);
            if (cm.language() == QStringLiteral("zh_CN")) {
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

    FullscreenWatcher fullscreenWatcher;
    QObject::connect(&fullscreenWatcher, &FullscreenWatcher::fullscreenAppStarted,
        &widget, [&widget] { widget.hide(); });
    QObject::connect(&fullscreenWatcher, &FullscreenWatcher::fullscreenAppStopped,
        &widget, [&widget] { widget.show(); });
    fullscreenWatcher.start();

    widget.show();
    return app.exec();
}

} // namespace bodyapp
