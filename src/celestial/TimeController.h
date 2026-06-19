#pragma once
#include <QObject>
#include <QTimer>

class SunModel;

// Recomputes the sun position each minute and triggers repaints at the fps cap.
class TimeController : public QObject {
    Q_OBJECT
public:
    TimeController(SunModel *sun, QObject *parent = nullptr);
    void setTarget(QObject *repaintTarget);
    void setFpsCap(int cap);
private slots:
    void onFrame();
private:
    SunModel *m_sun;
    QTimer m_frameTimer;
    QTimer m_sunTimer;
    QObject *m_target = nullptr;
};
