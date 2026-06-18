#include "globe/GlobeWindow.h"
#include "globe/ConfigManager.h"
#include "render/SunModel.h"
#include "render/CameraController.h"
#include "render/AssetManager.h"
#include "render/TimeController.h"
#include <QApplication>
#include <QStandardPaths>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Globe");

    ConfigManager config;
    AssetManager assets(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/textures");
    SunModel sun;
    CameraController camera;

    GlobeWindow w;
    w.view()->renderer().setAssets(&assets);
    w.view()->renderer().setSun(&sun);
    w.view()->renderer().setCamera(&camera);
    w.move(config.windowX(), config.windowY());
    w.resize(config.diameter(), config.diameter());

    TimeController time(&sun);
    time.setTarget(w.view());
    time.setFpsCap(config.fpsCap());

    w.show();
    return app.exec();
}
