#pragma once
#include <QMatrix4x4>

// Sun-centric base orientation (applied by the renderer from SunModel) plus a
// user yaw/pitch/zoom offset. drag & wheel manipulate the offset; reset()
// re-locks to the real-time sun-centric view.
class CameraController {
public:
    static constexpr float kMinZoom = 0.6f;
    static constexpr float kMaxZoom = 2.5f;

    void applyDrag(int dx, int dy);
    void applyZoom(int angleDelta);
    void reset();

    float offsetYaw() const { return m_yaw; }
    float offsetPitch() const { return m_pitch; }
    float zoom() const { return m_zoom; }

    // Extra rotation applied on top of the sun-centric base rotation.
    QMatrix4x4 offsetRotation() const;

private:
    float m_yaw = 0.0f;     // radians
    float m_pitch = 0.0f;   // radians, clamped
    float m_zoom = 1.0f;
};
