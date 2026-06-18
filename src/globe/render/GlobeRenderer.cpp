#include "GlobeRenderer.h"
#include <QOpenGLShader>
#include <QDateTime>
#include <QVector>
#include <cmath>

namespace {
constexpr double kPi = 3.14159265358979323846;
}

GlobeRenderer::GlobeRenderer() = default;

std::unique_ptr<QOpenGLShaderProgram> GlobeRenderer::makeProgram(const QString &vert, const QString &frag) {
    auto p = std::make_unique<QOpenGLShaderProgram>();
    p->addShaderFromSourceFile(QOpenGLShader::Vertex,   ":/shaders/" + vert);
    p->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shaders/" + frag);
    p->link();
    return p;
}

void GlobeRenderer::buildSphere(int stacks, int slices) {
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

void GlobeRenderer::loadTextures() {
    auto make = [](const QImage &img) {
        auto t = std::make_unique<QOpenGLTexture>(img.flipped(Qt::Vertical));
        t->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
        t->setMagnificationFilter(QOpenGLTexture::Linear);
        t->setWrapMode(QOpenGLTexture::Repeat);
        t->generateMipMaps();
        return t;
    };
    m_texDay   = make(m_assets->image(AssetManager::Day,    m_tierMaxSize));
    m_texNight = make(m_assets->image(AssetManager::Night,  m_tierMaxSize));
    m_texClouds= make(m_assets->image(AssetManager::Clouds, m_tierMaxSize));
}

QMatrix4x4 GlobeRenderer::sunCentricBaseRotation(double subSolarLatDeg, double subSolarLonDeg) {
    // QMatrix4x4::rotate() post-multiplies, so the calls below compose
    // M = Ry(lat-90) * Rz(-lon), i.e. Rz is applied to the vertex first.
    // Rz(-lon) brings the sub-solar longitude to the +X axis, leaving the
    // sub-solar point at (cosLat, 0, sinLat); Ry(lat-90) then maps it onto +Z.
    // Verified: maps sunDirection(lat,lon) -> (0,0,1) for all (lat,lon).
    QMatrix4x4 m;
    m.rotate(float(subSolarLatDeg) - 90.0f, 0.0f, 1.0f, 0.0f);
    m.rotate(-float(subSolarLonDeg),        0.0f, 0.0f, 1.0f);
    return m;
}

QMatrix4x4 GlobeRenderer::baseRotation() const {
    if (!m_sun) return {};
    if (m_useCenterLon)
        return sunCentricBaseRotation(0.0, m_centerLon); // home meridian at +Z
    return sunCentricBaseRotation(m_sun->subSolarLatitude(), m_sun->subSolarLongitude());
}

void GlobeRenderer::initialize(QOpenGLFunctions_3_3_Core *gl) {
    m_gl = gl;
    m_earthProg = makeProgram("earth.vert", "earth.frag");
    m_cloudProg = makeProgram("clouds.vert", "clouds.frag");
    m_atmoProg  = makeProgram("atmosphere.vert", "atmosphere.frag");
    buildSphere(128, 256);
    if (m_assets) loadTextures();
}

void GlobeRenderer::resize(int w, int h, float devicePixelRatio) {
    m_w = w; m_h = h; m_dpr = devicePixelRatio;
    m_aspect = 1.0f; // circular viewport uses min dimension
}

void GlobeRenderer::setAssets(AssetManager *a) {
    m_assets = a;
    if (m_gl && !m_texDay) loadTextures();
}

void GlobeRenderer::setQualityTier(int maxSize) {
    if (maxSize == m_tierMaxSize) return;
    m_tierMaxSize = maxSize;
    m_texDay.reset(); m_texNight.reset(); m_texClouds.reset();
    if (m_gl && m_assets) loadTextures();
}

void GlobeRenderer::render() {
    if (!m_gl) return;
    m_gl->glEnable(GL_MULTISAMPLE);
    m_gl->glEnable(GL_BLEND);
    m_gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    QMatrix4x4 proj;
    proj.perspective(35.0f, 1.0f, 0.1f, 10.0f);
    QMatrix4x4 view;
    view.translate(0.0f, 0.0f, -3.0f);

    const QMatrix4x4 base = baseRotation();
    const QMatrix4x4 offset = m_cam ? m_cam->offsetRotation() : QMatrix4x4();
    const float zoom = m_cam ? m_cam->zoom() : 1.0f;
    QMatrix4x4 model = offset * base;
    model.scale(zoom);

    const QVector3D sun = m_sun ? m_sun->sunDirection() : QVector3D(1, 0, 0);

    // Earth
    m_gl->glEnable(GL_DEPTH_TEST);
    m_earthProg->bind();
    m_earthProg->setUniformValue("uMVP", proj * view * model);
    m_earthProg->setUniformValue("uModel", model);
    m_earthProg->setUniformValue("uSunDir", sun);
    m_earthProg->setUniformValue("uHasDay", m_texDay ? 1.0f : 0.0f);
    m_earthProg->setUniformValue("uHasNight", m_texNight ? 1.0f : 0.0f);
    if (m_texDay)   { m_texDay->bind(0);   m_earthProg->setUniformValue("uDay", 0); }
    if (m_texNight) { m_texNight->bind(1); m_earthProg->setUniformValue("uNight", 1); }
    m_gl->glBindVertexArray(m_vao);
    m_gl->glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);

    // Clouds (slightly larger, slow spin)
    QMatrix4x4 cmodel = model;
    cmodel.scale(1.01f);
    const float spin = std::fmod(float(QDateTime::currentMSecsSinceEpoch()) * 1e-5f, 360.0f);
    cmodel.rotate(spin, 0.0f, 0.0f, 1.0f);
    m_cloudProg->bind();
    m_cloudProg->setUniformValue("uMVP", proj * view * cmodel);
    m_cloudProg->setUniformValue("uModel", cmodel);
    m_cloudProg->setUniformValue("uSunDir", sun);
    m_cloudProg->setUniformValue("uHasClouds", m_texClouds ? 1.0f : 0.0f);
    if (m_texClouds) { m_texClouds->bind(2); m_cloudProg->setUniformValue("uClouds", 2); }
    m_gl->glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);

    // Atmosphere (thin back-face shell). Keep depth test ON (enabled by the
    // earth pass) with depth writes off so the Earth occludes the inner shell
    // and only the silhouette limb glow is rasterized.
    m_gl->glDepthMask(GL_FALSE);
    QMatrix4x4 amodel = offset * baseRotation();
    amodel.scale(zoom * 1.015f);
    m_atmoProg->bind();
    m_atmoProg->setUniformValue("uMVP", proj * view * amodel);
    m_atmoProg->setUniformValue("uModel", amodel);
    m_atmoProg->setUniformValue("uSunDir", sun);
    m_atmoProg->setUniformValue("uViewPos", QVector3D(0, 0, 3));
    m_gl->glCullFace(GL_FRONT);
    m_gl->glEnable(GL_CULL_FACE);
    m_gl->glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);
    m_gl->glDisable(GL_CULL_FACE);
    m_gl->glDepthMask(GL_TRUE);
}
