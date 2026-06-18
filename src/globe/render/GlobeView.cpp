#include "GlobeView.h"
#include <QSurfaceFormat>

GlobeView::GlobeView(QWidget *parent) : QOpenGLWidget(parent) {
    QSurfaceFormat fmt;
    fmt.setVersion(3, 3);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setSamples(4); // MSAA
    fmt.setSwapInterval(1);
    setFormat(fmt);
}

GlobeView::~GlobeView() { makeCurrent(); }

void GlobeView::initializeGL() {
    m_gl = std::make_unique<QOpenGLFunctions_3_3_Core>();
    m_gl->initializeOpenGLFunctions();
    m_renderer.initialize(m_gl.get());
}

void GlobeView::resizeGL(int w, int h) {
    m_renderer.resize(w, h, float(devicePixelRatioF()));
}

void GlobeView::paintGL() {
    m_renderer.render();
}
