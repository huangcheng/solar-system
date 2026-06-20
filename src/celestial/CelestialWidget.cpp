#include "CelestialWidget.h"
#include "PlatformWindow.h"
#include "CameraController.h"
#include "ConfigManager.h"
#include <QRegion>
#include <QShowEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QOpenGLFunctions_3_3_Core>
#include <QSurfaceFormat>

CelestialWidget::CelestialWidget(const BodyConfig& config,
                                 ConfigManager *configManager,
                                 QWidget *parent)
    : QOpenGLWidget(parent), m_body(config), m_config(configManager) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint |
                   Qt::Tool | Qt::WindowDoesNotAcceptFocus);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
#ifdef Q_OS_MAC
    setAttribute(Qt::WA_MacAlwaysShowToolWindow, true);
#endif
    resize(220, 220);

    QSurfaceFormat fmt;
    fmt.setVersion(3, 3);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setSamples(4);
    fmt.setSwapInterval(1);
    setFormat(fmt);

    applyOptionsFromConfig();
}

CelestialWidget::~CelestialWidget() { makeCurrent(); }

void CelestialWidget::applyOptionsFromConfig() {
    if (!m_config) return;
    CelestialRenderOptions opts;
    opts.showGrid = m_config->showGrid();
    opts.useNightTexture = (m_config->nightMode() == QStringLiteral("texture"));
    opts.rotationSpeedRatio = m_config->rotationSpeed();
    opts.homeLat = m_config->homeLatitude();
    opts.homeLon = m_config->homeLongitude();
    // The home beacon is only meaningful once a real location fix has arrived,
    // and only if the user opted into location. Showing it at the default
    // (0,0) on a fresh install puts a pulsing dot in the Gulf of Guinea.
    opts.hasHome = m_config->locationOptIn() && m_hasLocationFix;
    m_body.setOptions(opts);
    setWindowFlag(Qt::WindowStaysOnTopHint, m_config->alwaysOnTop());
}

void CelestialWidget::setHomeLocation(double lat, double lon) {
    m_hasLocationFix = true;
    CelestialRenderOptions opts = m_body.options();
    opts.homeLat = lat;
    opts.homeLon = lon;
    // Respect the user's location opt-in: if they later disable location in
    // settings, applyOptionsFromConfig() will turn hasHome back off; but while
    // opted in, a fix enables the beacon immediately here.
    if (m_config && m_config->locationOptIn())
        opts.hasHome = true;
    m_body.setOptions(opts);
    update();
}

void CelestialWidget::setViewMode(ViewMode m) {
    if (m_viewMode == m) return;
    m_viewMode = m;   // set BEFORE resize() so the resizeEvent sees the new mode
    const int w = width();
    const QPoint c = geometry().center();
    if (m == ViewMode::FlatMap) {
        const int newH = qMax(120, w / 2);          // 2:1 keeping the width
        resize(w, newH);
        move(c - QPoint(w / 2, newH / 2));
        clearMask();
    } else {
        resize(w, w);                                // square again
        move(c - QPoint(w / 2, w / 2));
        // elliptical mask re-applied by resizeEvent()
    }
    update();
}

void CelestialWidget::initializeGL() {
    m_gl = std::make_unique<QOpenGLFunctions_3_3_Core>();
    if (!m_gl->initializeOpenGLFunctions())
        qWarning("CelestialWidget: failed to resolve OpenGL 3.3 core functions");
    m_body.initialize(m_gl.get());
}

void CelestialWidget::resizeGL(int w, int h) {
    m_body.resize(w, h, float(devicePixelRatioF()));
}

void CelestialWidget::paintGL() {
    if (m_viewMode == ViewMode::FlatMap)
        m_body.renderMap();
    else
        m_body.render();
}

void CelestialWidget::showEvent(QShowEvent *e) {
    QOpenGLWidget::showEvent(e);
    PlatformWindow::applyDwmFramelessAttributes(this);
}

void CelestialWidget::resizeEvent(QResizeEvent *e) {
    QOpenGLWidget::resizeEvent(e);
    if (m_viewMode == ViewMode::Globe) {
        const int side = qMin(width(), height());
        const QRect r((width() - side) / 2, (height() - side) / 2, side, side);
        setMask(QRegion(r, QRegion::Ellipse));
    } else {
        clearMask();   // flat map: rectangular widget
    }
}

void CelestialWidget::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton || e->button() == Qt::MiddleButton)
        m_lastPos = e->globalPosition().toPoint();
    QOpenGLWidget::mousePressEvent(e);
}

void CelestialWidget::mouseMoveEvent(QMouseEvent *e) {
    const QPoint g = e->globalPosition().toPoint();
    const bool moveGesture = (e->buttons() & Qt::MiddleButton) ||
                             ((e->buttons() & Qt::LeftButton) && (e->modifiers() & Qt::AltModifier));
    if (moveGesture) {
        window()->move(window()->pos() + g - m_lastPos);
        m_lastPos = g;
        m_moveGesture = true;  // defer config.save() to mouseReleaseEvent
    } else if (m_viewMode == ViewMode::Globe && (e->buttons() & Qt::LeftButton) && m_cam) {
        const QPoint d = g - m_lastPos;
        m_lastPos = g;
        m_cam->applyDrag(d.x(), d.y());
        update();
        emit userInteracted();  // let TimeController boost its repaint rate
    }
    QOpenGLWidget::mouseMoveEvent(e);
}

void CelestialWidget::mouseReleaseEvent(QMouseEvent *e) {
    // Persist the final window position once, on release, instead of on every
    // mouse-move pixel (which thrashed disk and stuttered the drag).
    if (m_moveGesture && m_config) {
        m_config->setWindowX(window()->pos().x());
        m_config->setWindowY(window()->pos().y());
        m_config->save();
    }
    m_moveGesture = false;
    QOpenGLWidget::mouseReleaseEvent(e);
}

void CelestialWidget::wheelEvent(QWheelEvent *e) {
    const int delta = e->angleDelta().y();
    const QPoint center = geometry().center();
    if (m_viewMode == ViewMode::FlatMap) {
        const int newW = qBound(240, int(width() + delta * 0.3), 1024);   // >=240 so height>=120
        if (newW == width()) { QOpenGLWidget::wheelEvent(e); return; }
        const int newH = qMax(120, newW / 2);
        resize(newW, newH);
        move(center - QPoint(newW / 2, newH / 2));
    } else {
        const int newSide = qBound(160, int(width() + delta * 0.3), 1024);
        if (newSide == width()) { QOpenGLWidget::wheelEvent(e); return; }
        resize(newSide, newSide);
        move(center - QPoint(newSide / 2, newSide / 2));
    }
    if (m_config) { m_config->setDiameter(width()); m_config->save(); }
    emit userInteracted();  // let TimeController boost its repaint rate
    update();
    QOpenGLWidget::wheelEvent(e);
}

void CelestialWidget::mouseDoubleClickEvent(QMouseEvent *e) {
    if (m_viewMode == ViewMode::FlatMap) { QOpenGLWidget::mouseDoubleClickEvent(e); return; }
    if (m_cam) {
        m_cam->reset();
        // Double-click is a camera-orientation reset, not a size reset: leave
        // the window at its current (possibly wheel-resized) diameter. Clear
        // useCenterLon for parity with the tray "Reset View" action.
        CelestialRenderOptions opts = m_body.options();
        opts.useCenterLon = false;
        m_body.setOptions(opts);
        update();
    }
    QOpenGLWidget::mouseDoubleClickEvent(e);
}
