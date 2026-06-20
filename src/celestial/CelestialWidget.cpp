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
    opts.hasHome = true;  // marker always shown at configured home
    m_body.setOptions(opts);
    setWindowFlag(Qt::WindowStaysOnTopHint, m_config->alwaysOnTop());
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
    m_body.render();
}

void CelestialWidget::showEvent(QShowEvent *e) {
    QOpenGLWidget::showEvent(e);
    PlatformWindow::applyDwmFramelessAttributes(this);
}

void CelestialWidget::resizeEvent(QResizeEvent *e) {
    QOpenGLWidget::resizeEvent(e);
    const int side = qMin(width(), height());
    const QRect r((width() - side) / 2, (height() - side) / 2, side, side);
    setMask(QRegion(r, QRegion::Ellipse));
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
    } else if ((e->buttons() & Qt::LeftButton) && m_cam) {
        const QPoint d = g - m_lastPos;
        m_lastPos = g;
        m_cam->applyDrag(d.x(), d.y());
        update();
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
    const int newSide = qBound(160, int(width() + delta * 0.3), 1024);
    if (newSide == width()) { QOpenGLWidget::wheelEvent(e); return; }
    const QPoint center = window()->geometry().center();
    window()->resize(newSide, newSide);
    window()->move(center - QPoint(newSide / 2, newSide / 2));
    if (m_config) {
        m_config->setDiameter(newSide);
        m_config->save();
    }
    update();
    QOpenGLWidget::wheelEvent(e);
}

void CelestialWidget::mouseDoubleClickEvent(QMouseEvent *e) {
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
