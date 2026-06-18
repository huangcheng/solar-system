#pragma once
#include <QWidget>
#include <QPoint>
#include "render/GlobeView.h"

class CameraController;
class ConfigManager;

class GlobeWindow : public QWidget {
    Q_OBJECT
public:
    explicit GlobeWindow(QWidget *parent = nullptr);
    GlobeView *view() { return m_view; }
    void setCameraController(CameraController *c) { m_cam = c; }
    void setConfig(ConfigManager *c) { m_config = c; }
protected:
    void paintEvent(QPaintEvent *) override {}
    void showEvent(QShowEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;
private:
    GlobeView *m_view;
    CameraController *m_cam = nullptr;
    ConfigManager *m_config = nullptr;
    QPoint m_lastPos;
};
