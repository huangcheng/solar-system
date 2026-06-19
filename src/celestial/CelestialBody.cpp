#include "CelestialBody.h"
#include <QOpenGLShader>
#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QCoreApplication>
#include <QIODevice>
#include <QVector>
#include <QVector3D>
#include <QVector4D>
#include <cmath>

namespace {
constexpr double kPi = 3.14159265358979323846;

// Load a GLSL source file from disk. Searches a few candidate directories so
// the same code works for an installed app (<exedir>/shaders) and a dev build
// (../src/celestial/shaders).
QByteArray loadShaderSource(const QString &name) {
    const QStringList dirs = {
        QCoreApplication::applicationDirPath() + "/shaders",
        QCoreApplication::applicationDirPath() + "/../src/celestial/shaders",
        QCoreApplication::applicationDirPath() + "/../../src/celestial/shaders",
    };
    for (const QString &d : dirs) {
        QFile f(d + "/" + name);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text))
            return f.readAll();
    }
    return {};
}
}

CelestialBody::CelestialBody(const BodyConfig& config)
    : m_config(config)
{
    // rotationSpeedRatio default lives in CelestialRenderOptions; keep it
    // consistent with the BodyConfig rotation period so a 24h body spins at
    // the same apparent rate as the legacy Earth widget.
}

void CelestialBody::setOptions(const CelestialRenderOptions& opts) {
    m_opts = opts;
    if (m_opts.rotationSpeedRatio < 1)
        m_opts.rotationSpeedRatio = 1;  // 1 = true 24h
}

std::unique_ptr<QOpenGLShaderProgram> CelestialBody::makeProgram(const QString &vert, const QString &frag) {
    auto p = std::make_unique<QOpenGLShaderProgram>();
    const QByteArray vsrc = loadShaderSource(vert);
    const QByteArray fsrc = loadShaderSource(frag);
    if (vsrc.isEmpty() || fsrc.isEmpty())
        qWarning().nospace() << "[shader] missing source: " << vert << "(" << vsrc.size() << ") " << frag << "(" << fsrc.size() << ")";
    p->addShaderFromSourceCode(QOpenGLShader::Vertex,   QString::fromUtf8(vsrc));
    p->addShaderFromSourceCode(QOpenGLShader::Fragment, QString::fromUtf8(fsrc));
    if (!p->link())
        qWarning().nospace() << "[shader] link failed for " << vert << "/" << frag << ": " << p->log();
    return p;
}

void CelestialBody::buildSphere(int stacks, int slices) {
    QVector<float> verts; QVector<GLuint> idx;
    for (int i = 0; i <= stacks; ++i) {
        const double lat = kPi * (-0.5 + double(i) / stacks);
        for (int j = 0; j <= slices; ++j) {
            const double lon = 2.0 * kPi * double(j) / slices;
            const float x = float(std::cos(lat) * std::cos(lon));
            const float y = float(std::cos(lat) * std::sin(lon));
            const float z = float(std::sin(lat));
            verts << x << y << z;
        }
    }
    const int stride = slices + 1;
    for (int i = 0; i < stacks; ++i)
        for (int j = 0; j < slices; ++j) {
            const int a = i * stride + j, b = a + 1, c = a + stride, d = c + 1;
            idx << GLuint(a) << GLuint(c) << GLuint(b)
                << GLuint(b) << GLuint(c) << GLuint(d);
        }
    m_indexCount = idx.size();

    m_gl->glGenVertexArrays(1, &m_vao);
    m_gl->glGenBuffers(1, &m_vbo);
    m_gl->glGenBuffers(1, &m_ibo);
    m_gl->glBindVertexArray(m_vao);
    m_gl->glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    m_gl->glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    m_gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
    m_gl->glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(GLuint), idx.data(), GL_STATIC_DRAW);
    m_gl->glEnableVertexAttribArray(0);
    m_gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    m_gl->glBindVertexArray(0);
}

void CelestialBody::loadTextures() {
    // Cap at 4K: the widget is small, and linear (no-mipmap) filtering on an
    // 8K texture would shimmer. 4K is plenty sharp for <= ~1000 px on screen.
    const int cap = qMin(m_tierMaxSize, 4096);
    auto make = [](const QImage &img) {
        auto t = std::make_unique<QOpenGLTexture>(img.flipped(Qt::Vertical));
        // Linear filtering, NO mipmaps. Mipmapping produces a dark vertical
        // seam at the longitude wrap (the UV derivative explodes across the
        // date line, forcing a 1x1 mip there). The equirectangular maps are
        // seamless, so linear filtering renders the wrap cleanly.
        t->setMinificationFilter(QOpenGLTexture::Linear);
        t->setMagnificationFilter(QOpenGLTexture::Linear);
        t->setWrapMode(QOpenGLTexture::Repeat);
        return t;
    };
    m_texDay      = make(m_assets->image(AssetManager::Day, cap));
    m_texNight    = make(m_assets->image(AssetManager::Night, cap));
    // Cloud layer disabled: at full opacity it hid the terrain. The earth should
    // read as bare land/mountains/sea. (clouds.* shaders are kept and the draw
    // pass self-skips when m_texClouds is null, so it's a one-line re-enable.)
    // m_texClouds  = make(m_assets->image(AssetManager::Clouds, cap));
    // Normal (terrain relief) + specular (ocean water mask) re-enabled. The old
    // seam came from a screen-space derivative tangent basis; earth.frag now
    // builds the tangent frame analytically from the sphere, and the glint is
    // water-masked + subtle (no glassy sheen).
    m_texNormal   = make(m_assets->image(AssetManager::Normal, cap));
    m_texSpecular = make(m_assets->image(AssetManager::Specular, cap));
}

QMatrix4x4 CelestialBody::sunCentricBaseRotation(double subSolarLatDeg, double subSolarLonDeg) {
    // Look-at orientation: point the camera at the sub-solar point while keeping
    // the Earth's spin axis UP on screen, so the globe never appears rolled/tilted
    // sideways. The earlier Ry(lat-90)*Rz(-lon) form centred the lit hemisphere
    // but left the "up" vector uncontrolled, which rolled the globe by the
    // sub-solar latitude (~23 deg at solstice).
    //
    // Build an orthonormal object-space frame at the sub-solar point and use it
    // as the matrix rows, mapping: east->+X, north-along-meridian->+Y, sun->+Z.
    // north pole then projects straight up (only tilted toward/away by season),
    // and sunDirection(lat,lon) still maps exactly to +Z (see test_sunview).
    const double latR = subSolarLatDeg * kPi / 180.0;
    const double lonR = subSolarLonDeg * kPi / 180.0;
    const QVector3D s(float(std::cos(latR) * std::cos(lonR)),
                      float(std::cos(latR) * std::sin(lonR)),
                      float(std::sin(latR)));
    const QVector3D polar(0.0f, 0.0f, 1.0f);             // object north pole
    QVector3D right = QVector3D::crossProduct(polar, s); // east at sub-solar point
    right.normalize();                                   // (|lat|<90 always, never degenerate)
    const QVector3D up = QVector3D::crossProduct(s, right); // north along the meridian (unit)

    QMatrix4x4 m;
    m.setRow(0, QVector4D(right, 0.0f));
    m.setRow(1, QVector4D(up,    0.0f));
    m.setRow(2, QVector4D(s,     0.0f));
    m.setRow(3, QVector4D(0.0f, 0.0f, 0.0f, 1.0f));
    return m;
}

void CelestialBody::initialize(QOpenGLFunctions_3_3_Core *gl) {
    m_gl = gl;
    m_earthProg = makeProgram("earth.vert", "earth.frag");
    m_cloudProg = makeProgram("clouds.vert", "clouds.frag");
    m_atmoProg  = makeProgram("atmosphere.vert", "atmosphere.frag");
    buildSphere(128, 256);
    if (m_assets) loadTextures();
}

void CelestialBody::resize(int w, int h, float devicePixelRatio) {
    m_w = w; m_h = h; m_dpr = devicePixelRatio;
    m_aspect = 1.0f; // circular viewport uses min dimension
}

void CelestialBody::setAssets(AssetManager *a) {
    m_assets = a;
    if (m_gl && !m_texDay) loadTextures();
}

void CelestialBody::setQualityTier(int maxSize) {
    if (maxSize == m_tierMaxSize) return;
    m_tierMaxSize = maxSize;
    m_texDay.reset(); m_texNight.reset(); m_texClouds.reset();
    m_texNormal.reset(); m_texSpecular.reset();
    if (m_gl && m_assets) loadTextures();
}

void CelestialBody::render() {
    if (!m_gl) return;
    // Opaque "space" background — the widget is a solid disc, not see-through.
    m_gl->glClearColor(0.02f, 0.03f, 0.05f, 1.0f);
    m_gl->glEnable(GL_MULTISAMPLE);
    m_gl->glEnable(GL_BLEND);
    m_gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    QMatrix4x4 proj;
    proj.perspective(35.0f, 1.0f, 0.1f, 10.0f);
    QMatrix4x4 view;
    view.translate(0.0f, 0.0f, -3.25f);   // globe fills the whole disc — no black rim

    // Day/night follows the REAL wall clock. SunModel gives the true sub-solar
    // point (it drifts ~15 deg/hour), so the globe always shows Earth's ACTUAL
    // current day/night -- not a time-lapse. The sun-centric framing points the
    // lit hemisphere at the camera, so the disc always reads like the real planet
    // (about half lit) and never wanders onto the dark side.
    const double subLat = m_sun ? m_sun->subSolarLatitude()  : 0.0;
    const double subLon = m_sun ? m_sun->subSolarLongitude() : 0.0;
    const QMatrix4x4 base = m_opts.useCenterLon
            ? sunCentricBaseRotation(0.0, m_opts.centerLon)     // home meridian at +Z
            : sunCentricBaseRotation(subLat, subLon);           // sub-solar at +Z
    const QMatrix4x4 offset = m_cam ? m_cam->offsetRotation() : QMatrix4x4();
    const float zoom = m_cam ? m_cam->zoom() : 1.0f;
    // Tilt the sun ~50deg off the camera axis so the terminator reads as a
    // visible crescent instead of the sun being pinned dead-centre.
    QMatrix4x4 tilt; tilt.rotate(50.0f, 0.0f, 1.0f, 0.0f);
    const QMatrix4x4 oriented = offset * tilt * base;

    // Static by default (the globe shows the real "now"). rotationSpeed > 1 adds
    // an optional polar-axis spin for visible motion: continents drift, but the
    // sun stays fixed in world (sunWorld below omits the spin), so the day/night
    // split and the mostly-lit framing are unaffected -- it never spins into night.
    const int rotRatio = m_opts.rotationSpeedRatio;
    QMatrix4x4 model = oriented;
    if (rotRatio > 1) {
        const double spinDeg = std::fmod(double(QDateTime::currentMSecsSinceEpoch())
                                        * (360.0 / 86400000.0) * double(rotRatio), 360.0);
        QMatrix4x4 spin; spin.rotate(float(spinDeg), 0.0f, 0.0f, 1.0f);
        model = oriented * spin;
    }
    model.scale(1.0f);   // globe always fills the widget disc — wheel resizes the widget, not the 3D model

    const QVector3D sun = m_sun ? m_sun->sunDirection() : QVector3D(1, 0, 0);
    // Derive sunWorld from the FULL model (including spin) so the day/night
    // terminator is "painted" onto the globe surface: it stays glued to the
    // same longitudes and rotates WITH the Earth like a toy globe you spin.
    // (dot(model*aPos, model*sun) == dot(aPos, sun) — spin cancels out, so
    // each continent keeps its real-world day/night status as it spins.)
    const QVector3D sunWorld = model.mapVector(sun).normalized();

    // Earth
    m_gl->glEnable(GL_DEPTH_TEST);
    m_earthProg->bind();
    m_earthProg->setUniformValue("uMVP", proj * view * model);
    m_earthProg->setUniformValue("uModel", model);
    m_earthProg->setUniformValue("uSunWorld", sunWorld);
    m_earthProg->setUniformValue("uViewPos", QVector3D(0, 0, 3.25));
    m_earthProg->setUniformValue("uHasDay", m_texDay ? 1.0f : 0.0f);
    m_earthProg->setUniformValue("uHasNight", m_texNight ? 1.0f : 0.0f);
    m_earthProg->setUniformValue("uHasNormal", m_texNormal ? 1.0f : 0.0f);
    m_earthProg->setUniformValue("uHasSpecular", m_texSpecular ? 1.0f : 0.0f);
    m_earthProg->setUniformValue("uShowGrid", m_opts.showGrid ? 1.0f : 0.0f);
    m_earthProg->setUniformValue("uUseNightTexture", m_opts.useNightTexture ? 1.0f : 0.0f);
    if (m_texDay)     { m_texDay->bind(0);       m_earthProg->setUniformValue("uDay", 0); }
    if (m_texNight)   { m_texNight->bind(1);     m_earthProg->setUniformValue("uNight", 1); }
    if (m_texNormal)  { m_texNormal->bind(3);    m_earthProg->setUniformValue("uNormal", 3); }
    if (m_texSpecular){ m_texSpecular->bind(4);  m_earthProg->setUniformValue("uSpecular", 4); }
    // Pulsing "you are here" marker (ECEF direction of the granted location).
    {
        const double la = qDegreesToRadians(m_opts.homeLat), lo = qDegreesToRadians(m_opts.homeLon);
        const QVector3D homeDir(float(std::cos(la) * std::cos(lo)),
                                float(std::cos(la) * std::sin(lo)),
                                float(std::sin(la)));
        m_earthProg->setUniformValue("uHomeDir", homeDir);
        m_earthProg->setUniformValue("uHasHome", m_opts.hasHome ? 1.0f : 0.0f);
        // Keep uTime small (mod 10 min) so float seconds-since-epoch stays precise.
        m_earthProg->setUniformValue("uTime", float(std::fmod(QDateTime::currentMSecsSinceEpoch() / 1000.0, 600.0)));
    }
    m_gl->glBindVertexArray(m_vao);
    m_gl->glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);

    // Clouds (slightly larger, slow spin) — skipped when the layer is disabled.
    if (m_texClouds) {
        QMatrix4x4 cmodel = model;
        cmodel.scale(1.01f);
        const float spin = std::fmod(float(QDateTime::currentMSecsSinceEpoch()) * 1e-5f, 360.0f);
        cmodel.rotate(spin, 0.0f, 0.0f, 1.0f);
        m_cloudProg->bind();
        m_cloudProg->setUniformValue("uMVP", proj * view * cmodel);
        m_cloudProg->setUniformValue("uModel", cmodel);
        m_cloudProg->setUniformValue("uSunWorld", sunWorld);
        m_cloudProg->setUniformValue("uHasClouds", 1.0f);
        m_texClouds->bind(2);
        m_cloudProg->setUniformValue("uClouds", 2);
        m_gl->glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
    }

    // Atmosphere (thin back-face shell). Keep depth test ON (enabled by the
    // earth pass) with depth writes off so the Earth occludes the inner shell
    // and only the silhouette limb glow is rasterized.
    m_gl->glDepthMask(GL_FALSE);
    QMatrix4x4 amodel = oriented;
    amodel.scale(1.02f);
    m_atmoProg->bind();
    m_atmoProg->setUniformValue("uMVP", proj * view * amodel);
    m_atmoProg->setUniformValue("uModel", amodel);
    m_atmoProg->setUniformValue("uSunDir", sunWorld);
    m_atmoProg->setUniformValue("uViewPos", QVector3D(0, 0, 3.25));
    m_gl->glCullFace(GL_FRONT);
    m_gl->glEnable(GL_CULL_FACE);
    m_gl->glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
    m_gl->glDisable(GL_CULL_FACE);
    m_gl->glDepthMask(GL_TRUE);
}
