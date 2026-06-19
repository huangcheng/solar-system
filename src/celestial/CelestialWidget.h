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
    void wheelEvent(QWheelEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;

private:
    CelestialBody m_body;
    ConfigManager *m_config = nullptr;
    CameraController *m_cam = nullptr;
    QPoint m_lastPos;
    std::unique_ptr<QOpenGLFunctions_3_3_Core> m_gl;
};
