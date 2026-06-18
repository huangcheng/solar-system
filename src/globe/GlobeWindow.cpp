#include "GlobeWindow.h"
#include "PlatformWindow.h"
#include "ConfigManager.h"
#include "render/CameraController.h"
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
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_NoSystemBackground);
#ifdef Q_OS_MAC
    setAttribute(Qt::WA_MacAlwaysShowToolWindow, true);
#endif
    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(m_view);
    resize(360, 360);
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
    if (m_cam) {
        m_cam->applyZoom(e->angleDelta().y());
        const int side = int(360 * m_cam->zoom());
        resize(side, side);
        m_view->update();
    }
    QWidget::wheelEvent(e);
}

void GlobeWindow::mouseDoubleClickEvent(QMouseEvent *e) {
    if (m_cam) {
        m_cam->reset();
        resize(360, 360);
        m_view->update();
    }
    QWidget::mouseDoubleClickEvent(e);
}
