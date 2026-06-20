#include "TimeController.h"
#include "SunModel.h"
#include <QDateTime>
#include <QMetaObject>

TimeController::TimeController(SunModel *sun, QObject *parent)
    : QObject(parent), m_sun(sun) {
    m_sun->setUtc(QDateTime::currentDateTimeUtc());
    connect(&m_sunTimer, &QTimer::timeout, this, [this] {
        m_sun->setUtc(QDateTime::currentDateTimeUtc());
    });
    m_sunTimer.start(60000); // recompute sun every minute

    m_boostTimer.setSingleShot(true);
    m_boostTimer.setInterval(2000); // ~2s of high-rate repaint after input
    connect(&m_boostTimer, &QTimer::timeout, this, [this] {
        m_boosted = false;
        applyRate();
    });

    connect(&m_frameTimer, &QTimer::timeout, this, &TimeController::onFrame);
    setFpsCap(60);  // sets m_highInterval, then applyRate() starts the frame timer
}

void TimeController::setTarget(QObject *target) { m_target = target; }

void TimeController::setFpsCap(int cap) {
    m_highInterval = (cap == 30) ? 33 : 16;
    applyRate();
}

void TimeController::setAnimating(bool animating) {
    if (m_animating == animating) return;
    m_animating = animating;
    applyRate();
}

void TimeController::boost() {
    m_boosted = true;
    applyRate();
    m_boostTimer.start();  // (re)start the 2s high-rate window
}

int TimeController::baselineInterval() const {
    // Idle baseline in Real-time mode: ~5 fps is plenty for the slow sun drift
    // (~0.0007 deg/frame at 60fps). Animating (spinning) bodies need the full
    // fps cap to look smooth.
    return m_animating ? m_highInterval : 200;
}

void TimeController::applyRate() {
    const int interval = m_boosted ? m_highInterval : baselineInterval();
    m_frameTimer.start(interval);
}

void TimeController::onFrame() {
    if (m_target)
        QMetaObject::invokeMethod(m_target, "update", Qt::QueuedConnection);
}
