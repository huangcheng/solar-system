#pragma once
#include <QOpenGLWidget>
#include <memory>
#include "GlobeRenderer.h"

class AssetManager;
class SunModel;
class CameraController;

class GlobeView : public QOpenGLWidget {
    Q_OBJECT
public:
    explicit GlobeView(QWidget *parent = nullptr);
    ~GlobeView() override;

    GlobeRenderer &renderer() { return m_renderer; }

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    std::unique_ptr<QOpenGLFunctions_3_3_Core> m_gl;
    GlobeRenderer m_renderer;
};
