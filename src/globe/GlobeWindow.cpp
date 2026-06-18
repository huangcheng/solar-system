#include "GlobeWindow.h"
#include <QVBoxLayout>

GlobeWindow::GlobeWindow(QWidget *parent)
    : QWidget(parent), m_view(new GlobeView(this)) {
    setAttribute(Qt::WA_TranslucentBackground);
    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(m_view);
    resize(360, 360);
}
