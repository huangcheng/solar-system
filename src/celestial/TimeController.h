#pragma once
#include <QObject>
#include <QTimer>

class SunModel;

// Recomputes the sun position each minute and triggers repaints at an adaptive
// rate. When the body is animating (spinning faster than real-time) it repaints
// at the configured fps cap; when idle in Real-time mode it drops to a low
// baseline (~5 fps) that's still enough to track the slow sun drift. boost()
// from user input snaps back to the high rate for a short window so drags and
// wheel zooms stay smooth.
class TimeController : public QObject {
    Q_OBJECT
public:
    TimeController(SunModel *sun, QObject *parent = nullptr);
    void setTarget(QObject *repaintTarget);
    void setFpsCap(int cap);
    // Sets whether the body is spinning faster than real-time. While animating,
    // the baseline repaint rate is the fps cap (needed for smooth spin);
    // otherwise it drops to the low idle rate.
    void setAnimating(bool animating);
    // Snaps the repaint rate up to the fps cap for a short window after user
    // input, then returns to the baseline.
    void boost();
private slots:
    void onFrame();
private:
    void applyRate();             // restarts m_frameTimer at the effective interval
    int baselineInterval() const; // high rate when animating, low rate otherwise
    SunModel *m_sun;
    QTimer m_frameTimer;
    QTimer m_sunTimer;
    QTimer m_boostTimer;          // single-shot: clears m_boosted after ~2s
    QObject *m_target = nullptr;
    int m_highInterval = 16;      // from fps cap (16ms = 60fps, 33ms = 30fps)
    bool m_animating = false;     // baseline uses the high rate when true
    bool m_boosted = false;       // temporarily using the high rate after input
};
