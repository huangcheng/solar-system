#pragma once
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QMatrix4x4>
#include <QVector3D>
#include <memory>
#include "BodyConfig.h"
#include "AssetManager.h"
#include "SunModel.h"
#include "CameraController.h"

// Render-time switches for CelestialBody. Packaged into one struct so callers
// (and later phases) can set the whole state atomically; CelestialBody itself
// stays free of per-feature setters.
struct CelestialRenderOptions {
    bool showGrid = false;
    bool useNightTexture = false;
    bool hasHome = false;
    double homeLat = 0.0;
    double homeLon = 0.0;
    double centerLon = 0.0;
    bool useCenterLon = false;
    int rotationSpeedRatio = 2880;  // 1 = real-time (one turn / 24h)
};

// Generic celestial sphere renderer. Phase 1 is a near-verbatim port of
// GlobeRenderer: it renders an Earth-shaped body (day/night/clouds/atmosphere)
// using a BodyConfig for identity and an AssetManager for textures. Later
// phases will generalize the shading for other body types (gas giants, stars,
// ringed planets) without changing this public interface.
class CelestialBody {
public:
    explicit CelestialBody(const BodyConfig& config);

    void initialize(QOpenGLFunctions_3_3_Core *gl);
    void setAssets(AssetManager *a);
    void setSun(const SunModel *s) { m_sun = s; }
    void setCamera(const CameraController *c) { m_cam = c; }
    void setOptions(const CelestialRenderOptions& opts);
    void setQualityTier(int maxSize);

    const CelestialRenderOptions& options() const { return m_opts; }
    const BodyConfig& config() const { return m_config; }

    void resize(int w, int h, float devicePixelRatio);
    void render();

    // Pure, GL-free sun-centric rotation: maps the sub-solar point (lat,lon)
    // onto +Z so the lit hemisphere faces the camera. Unit-testable.
    static QMatrix4x4 sunCentricBaseRotation(double subSolarLatDeg, double subSolarLonDeg);

private:
    BodyConfig m_config;
    QOpenGLFunctions_3_3_Core *m_gl = nullptr;
    AssetManager *m_assets = nullptr;
    const SunModel *m_sun = nullptr;
    const CameraController *m_cam = nullptr;
    CelestialRenderOptions m_opts;
    int m_tierMaxSize = 8192;

    std::unique_ptr<QOpenGLShaderProgram> m_earthProg, m_cloudProg, m_atmoProg;
    std::unique_ptr<QOpenGLShaderProgram> m_starProg;   // emissive (Sun) — null for planets
    std::unique_ptr<QOpenGLShaderProgram> m_ringProg;   // Saturn rings — null unless hasRings
    std::unique_ptr<QOpenGLTexture> m_texDay, m_texNight, m_texClouds, m_texNormal, m_texSpecular;
    std::unique_ptr<QOpenGLTexture> m_texRing;          // Saturn ring texture
    GLuint m_vao = 0, m_vbo = 0, m_ibo = 0;
    int m_indexCount = 0;
    GLuint m_ringVao = 0, m_ringVbo = 0, m_ringIbo = 0;
    int m_ringIndexCount = 0;
    float m_aspect = 1.0f;
    float m_dpr = 1.0f;
    int m_w = 0, m_h = 0;

    void buildSphere(int stacks, int slices);
    void buildRingQuad();          // annulus in the equatorial (XY) plane
    void loadTextures();
    std::unique_ptr<QOpenGLShaderProgram> makeProgram(const QString &vertAlias, const QString &fragAlias);
};
