#pragma once
#include <QWidget>
#include <QPoint>
#include <QOpenGLWidget>
#include <memory>
#include "BodyConfig.h"
#include "CelestialBody.h"

class CameraController;
class ConfigManager;
class QOpenGLFunctions_3_3_Core;

class CelestialWidget : public QOpenGLWidget {
    Q_OBJECT
public:
    explicit CelestialWidget(const BodyConfig& config,
                             ConfigManager *configManager,
                             QWidget *parent = nullptr);
    ~CelestialWidget() override;

    CelestialBody& body() { return m_body; }

    void setConfig(ConfigManager *c) { m_config = c; }
    void setCameraController(CameraController *c) { m_cam = c; m_body.setCamera(c); }

    void applyOptionsFromConfig();

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void showEvent(QShowEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;

private:
    // m_gl must outlive m_body: CelestialBody's destructor tears down GL
    // resources through the raw pointer it holds. Members destruct in reverse
    // declaration order, so m_gl is declared first (destroyed last).
    std::unique_ptr<QOpenGLFunctions_3_3_Core> m_gl;
    CelestialBody m_body;
    ConfigManager *m_config = nullptr;
    CameraController *m_cam = nullptr;
    QPoint m_lastPos;
    bool m_moveGesture = false;  // true while an Alt-drag/middle-drag window move is in progress
};
