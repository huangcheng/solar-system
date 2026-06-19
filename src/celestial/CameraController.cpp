#include "CameraController.h"
#include <QtMath>

void CameraController::applyDrag(int dx, int dy) {
    m_yaw   += dx * 0.005f;
    m_pitch += dy * 0.005f;
    const float lim = 1.45f; // ~83 deg
    if (m_pitch > lim) m_pitch = lim;
    if (m_pitch < -lim) m_pitch = -lim;
}

void CameraController::applyZoom(int angleDelta) {
    m_zoom *= std::pow(1.0015f, float(angleDelta));
    if (m_zoom < kMinZoom) m_zoom = kMinZoom;
    if (m_zoom > kMaxZoom) m_zoom = kMaxZoom;
}

void CameraController::reset() {
    m_yaw = 0.0f; m_pitch = 0.0f; m_zoom = 1.0f;
}

QMatrix4x4 CameraController::offsetRotation() const {
    QMatrix4x4 r;
    r.rotate(qRadiansToDegrees(m_pitch), 1.0f, 0.0f, 0.0f);
    r.rotate(qRadiansToDegrees(m_yaw),   0.0f, 1.0f, 0.0f);
    return r;
}
