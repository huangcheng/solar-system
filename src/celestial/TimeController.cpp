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
    setFpsCap(60);
    connect(&m_frameTimer, &QTimer::timeout, this, &TimeController::onFrame);
}

void TimeController::setTarget(QObject *target) { m_target = target; }

void TimeController::setFpsCap(int cap) {
    const int interval = (cap == 30) ? 33 : 16;
    m_frameTimer.start(interval);
}

void TimeController::onFrame() {
    if (m_target)
        QMetaObject::invokeMethod(m_target, "update", Qt::QueuedConnection);
}
