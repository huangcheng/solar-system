// Offscreen render harness — NOT a ctest. Renders one globe frame to a PNG so a
// shader/lighting change can be inspected without launching the desktop widget.
//
//   render_shot <out.png> [centerLon=20] [utcIso=2026-06-19T13:20:00Z] [pitchDeg=0] [yawDeg=0]
//
// Defaults reproduce the reported view: Africa face-on, sub-solar point up-left,
// terminator curving down the right and wrapping to the lower-left ocean limb
// (the spot where the old broad twilight band smeared into a pink blob).
// pitch/yaw tilt the camera (e.g. pitch to bring a pole into view).
#include <QGuiApplication>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLFramebufferObject>
#include <QDateTime>
#include <QImage>
#include <QDebug>
#include <cmath>

#include "render/GlobeRenderer.h"
#include "render/AssetManager.h"
#include "render/SunModel.h"
#include "render/CameraController.h"

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);

    const QString out = argc > 1 ? QString::fromLocal8Bit(argv[1])
                                 : QStringLiteral("shot.png");
    const double centerLon = argc > 2 ? QString::fromLocal8Bit(argv[2]).toDouble() : 20.0;
    const QString utcIso = argc > 3 ? QString::fromLocal8Bit(argv[3])
                                    : QStringLiteral("2026-06-19T13:20:00Z");
    const double pitchDeg = argc > 4 ? QString::fromLocal8Bit(argv[4]).toDouble() : 0.0;
    const double yawDeg   = argc > 5 ? QString::fromLocal8Bit(argv[5]).toDouble() : 0.0;
    const int    size     = argc > 6 ? QString::fromLocal8Bit(argv[6]).toInt()    : 360;
    const double zoom     = argc > 7 ? QString::fromLocal8Bit(argv[7]).toDouble() : 1.0;

    QSurfaceFormat fmt;
    fmt.setVersion(3, 3);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setSamples(4);
    QSurfaceFormat::setDefaultFormat(fmt);

    QOffscreenSurface surface;
    surface.setFormat(fmt);
    surface.create();
    if (!surface.isValid()) { qWarning("offscreen surface invalid"); return 2; }

    QOpenGLContext ctx;
    ctx.setFormat(fmt);
    if (!ctx.create()) { qWarning("GL context create failed"); return 2; }
    if (!ctx.makeCurrent(&surface)) { qWarning("makeCurrent failed"); return 2; }

    QOpenGLFunctions_3_3_Core gl;
    if (!gl.initializeOpenGLFunctions()) { qWarning("no GL 3.3 core"); return 2; }

    QOpenGLFramebufferObjectFormat ffmt;
    ffmt.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    ffmt.setSamples(4);
    QOpenGLFramebufferObject fbo(QSize(size, size), ffmt);
    fbo.bind();
    gl.glViewport(0, 0, size, size);

    AssetManager assets;
    SunModel sun;
    sun.setUtc(QDateTime::fromString(utcIso, Qt::ISODate));
    CameraController cam;
    // applyDrag uses 0.005 rad per unit; convert requested degrees to drag units.
    const double k = (3.14159265358979 / 180.0) / 0.005;
    cam.applyDrag(int(yawDeg * k), int(pitchDeg * k));
    // applyZoom multiplies by 1.0015^angle; solve angle for the requested zoom.
    if (zoom != 1.0) cam.applyZoom(int(std::log(zoom) / std::log(1.0015)));

    GlobeRenderer r;
    r.setAssets(&assets);
    r.setSun(&sun);
    r.setCamera(&cam);
    r.initialize(&gl);
    r.resize(size, size, 1.0f);
    // centerLon == 1000 -> leave sun-centric (the real app default); otherwise
    // lock the given meridian to the camera (the tray "center on me" mode).
    if (centerLon < 999.0) r.setCenterLongitude(centerLon);
    r.render();
    gl.glFinish();

    const QImage img = fbo.toImage();
    if (!img.save(out)) { qWarning() << "save failed:" << out; return 3; }
    qInfo().noquote() << "wrote" << out
                      << "subSolar(lat,lon)=" << sun.subSolarLatitude()
                      << sun.subSolarLongitude();
    return 0;
}
