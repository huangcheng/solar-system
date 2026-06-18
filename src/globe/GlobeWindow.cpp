#include "GlobeWindow.h"
#include <QPainter>
#include <QPaintEvent>

GlobeWindow::GlobeWindow(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground);
    resize(360, 360);
}

void GlobeWindow::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(QColor(20, 40, 90));
    p.setPen(Qt::NoPen);
    p.drawEllipse(rect());
}
