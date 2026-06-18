#include "GlobeWindow.h"
#include "PlatformWindow.h"
#include <QVBoxLayout>
#include <QRegion>
#include <QShowEvent>
#include <QResizeEvent>

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
