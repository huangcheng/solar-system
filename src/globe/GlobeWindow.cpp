#include "GlobeWindow.h"
#include "PlatformWindow.h"
#include "ConfigManager.h"
#include "celestial/CameraController.h"
#include <QVBoxLayout>
#include <QRegion>
#include <QShowEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QWheelEvent>

GlobeWindow::GlobeWindow(QWidget *parent)
    : QWidget(parent), m_view(new GlobeView(this)) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint |
                   Qt::Tool | Qt::WindowDoesNotAcceptFocus);
    setAttribute(Qt::WA_ShowWithoutActivating);
    // Opaque widget (solid disc on the desktop), not transparent.
    setAttribute(Qt::WA_OpaquePaintEvent, true);
#ifdef Q_OS_MAC
    setAttribute(Qt::WA_MacAlwaysShowToolWindow, true);
#endif
    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(m_view);
    resize(220, 220);
}

void GlobeWindow::showEvent(QShowEvent *e) {
    QWidget::showEvent(e);
    PlatformWindow::applyDwmFramelessAttributes(this);
}

void GlobeWindow::resizeEvent(QResizeEvent *e) {
    QWidget::resizeEvent(e);
    const int side = qMin(width(), height());
    const QRect r((width() - side) / 2, (height() - side) / 2, side, side);
    setMask(QRegion(r, QRegion::Ellipse));
}

void GlobeWindow::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton || e->button() == Qt::MiddleButton)
        m_lastPos = e->globalPosition().toPoint();
    QWidget::mousePressEvent(e);
}

void GlobeWindow::mouseMoveEvent(QMouseEvent *e) {
    const QPoint g = e->globalPosition().toPoint();
    const bool moveGesture = (e->buttons() & Qt::MiddleButton) ||
                             ((e->buttons() & Qt::LeftButton) && (e->modifiers() & Qt::AltModifier));
    if (moveGesture) {
        move(pos() + g - m_lastPos);
        m_lastPos = g;
        if (m_config) {
            m_config->setWindowX(pos().x());
            m_config->setWindowY(pos().y());
            m_config->save();
        }
    } else if ((e->buttons() & Qt::LeftButton) && m_cam) {
        const QPoint d = g - m_lastPos;
        m_lastPos = g;
        m_cam->applyDrag(d.x(), d.y());
        m_view->update();
    }
    QWidget::mouseMoveEvent(e);
}

void GlobeWindow::wheelEvent(QWheelEvent *e) {
    // Wheel resizes the ENTIRE globe widget (not a 3D magnifier zoom).
    // The globe always fills the circular widget at any size.
    const int delta = e->angleDelta().y();
    const int newSide = qBound(160, int(width() + delta * 0.3), 1024);
    if (newSide == width()) { QWidget::wheelEvent(e); return; }
    const QPoint center = geometry().center();
    resize(newSide, newSide);
    move(center - QPoint(newSide / 2, newSide / 2));
    if (m_config) {
        m_config->setDiameter(newSide);
        m_config->save();
    }
    m_view->update();
    QWidget::wheelEvent(e);
}

void GlobeWindow::mouseDoubleClickEvent(QMouseEvent *e) {
    if (m_cam) {
        m_cam->reset();
        const QPoint center = geometry().center();
        resize(220, 220);
        move(center - QPoint(110, 110));
        if (m_config) {
            m_config->setDiameter(220);
            m_config->save();
        }
        m_view->update();
    }
    QWidget::mouseDoubleClickEvent(e);
}
