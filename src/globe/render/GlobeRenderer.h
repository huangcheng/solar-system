#pragma once
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QMatrix4x4>
#include <QVector3D>
#include <memory>
#include "AssetManager.h"
#include "SunModel.h"
#include "CameraController.h"

class GlobeRenderer {
public:
    GlobeRenderer();
    void initialize(QOpenGLFunctions_3_3_Core *gl);
    void resize(int w, int h, float devicePixelRatio);
    void setAssets(AssetManager *a);
    void setSun(const SunModel *s) { m_sun = s; }
    void setCamera(const CameraController *c) { m_cam = c; }
    void setRotationSpeedRatio(int r) { m_rotRatio = (r < 1 ? 1 : r); } // 1 = true 24h
    void setQualityTier(int maxSize);
    void render();

    // "Center on me": lock the view over a specific longitude (home meridian)
    // at the equator, while lighting still reflects the real sun direction.
    // clearCenterLongitude() returns to the sun-centric default.
    void setCenterLongitude(double lon) { m_centerLon = lon; m_useCenterLon = true; }
    void clearCenterLongitude() { m_useCenterLon = false; }

    // Pure, GL-free sun-centric rotation: maps the sub-solar point (lat,lon)
    // onto +Z so the lit hemisphere faces the camera. Unit-testable.
    static QMatrix4x4 sunCentricBaseRotation(double subSolarLatDeg, double subSolarLonDeg);

private:
    QOpenGLFunctions_3_3_Core *m_gl = nullptr;
    AssetManager *m_assets = nullptr;
    const SunModel *m_sun = nullptr;
    const CameraController *m_cam = nullptr;
    int m_rotRatio = 960;   // rotation x real-time (960 -> ~1 turn / 90 s)
    int m_tierMaxSize = 8192;
    double m_centerLon = 0.0;
    bool m_useCenterLon = false;

    std::unique_ptr<QOpenGLShaderProgram> m_earthProg, m_cloudProg, m_atmoProg;
    std::unique_ptr<QOpenGLTexture> m_texDay, m_texNight, m_texClouds, m_texNormal, m_texSpecular;
    GLuint m_vao = 0, m_vbo = 0, m_ibo = 0;
    int m_indexCount = 0;
    float m_aspect = 1.0f;
    float m_dpr = 1.0f;
    int m_w = 0, m_h = 0;

    void buildSphere(int stacks, int slices);
    void loadTextures();
    std::unique_ptr<QOpenGLShaderProgram> makeProgram(const QString &vertAlias, const QString &fragAlias);
    QMatrix4x4 baseRotation() const; // sun-centric from SunModel
};
