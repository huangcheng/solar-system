# Flat Map Mode Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a "Flat Map" toggle to the Earth widget that reshapes the circular 3D globe into a 2:1 equirectangular flat map of the whole world, rendering the existing day texture flat with a live real-time day/night terminator driven by the true wall-clock sun (no spin).

**Architecture:** A new flat-map GL render path (new `map.vert`/`map.frag` + a fullscreen quad + `CelestialBody::renderMap()`), toggled at the widget level (`CelestialWidget::ViewMode`), persisted via `ConfigManager.viewMode`, exposed via a checkable tray action. The map shader samples the equirectangular day/night textures directly by UV and computes day/night per pixel from the real ECEF sun — the same lighting math as the globe, minus the spin.

**Tech Stack:** Qt6, OpenGL 3.3 Core, existing `celestial` library + shaders, existing `SunModel`.

---

## File Structure

```
src/celestial/
├── ConfigManager.h/.cpp        (modified: + viewMode)
├── CelestialBody.h/.cpp        (modified: + renderMap(), m_mapProg, quad geometry)
├── CelestialWidget.h/.cpp      (modified: + ViewMode, setViewMode, branch paint/resize/wheel)
├── SystemTray.h/.cpp           (modified: + checkable "Flat Map" action + signal)
├── TerminatorModel.h/.cpp      (NEW — GL-free day/night helper, unit-testable)
└── shaders/
    ├── map.vert                (NEW)
    └── map.frag                (NEW)
src/apps/earth/main.cpp          (modified: wire toggle + persisted startup mode)
tests/
├── test_configmanager.cpp       (modified: + viewMode round-trip)
├── test_terminator.cpp          (NEW)
└── CMakeLists.txt               (modified: + test_terminator)
```

Build/test commands (set PATH first each session):
```bash
$env:PATH = "C:\Qt\6.11.1\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\CMake_64\bin;C:\Qt\Tools\Ninja;" + $env:PATH
```

---

## Task 1: Add `viewMode` to `ConfigManager` (TDD)

**Files:**
- Modify: `src/celestial/ConfigManager.h`
- Modify: `src/celestial/ConfigManager.cpp`
- Modify: `tests/test_configmanager.cpp`

- [ ] **Step 1: Add the failing test**

In `tests/test_configmanager.cpp`, add a slot (use the existing hermetic `ConfigManager(dir.path())` pattern already in that file):

```cpp
void testViewModeRoundTrip() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    ConfigManager cm(dir.path());
    QCOMPARE(cm.viewMode(), QStringLiteral("globe"));   // default

    cm.setViewMode(QStringLiteral("map"));
    QCOMPARE(cm.viewMode(), QStringLiteral("map"));
    cm.save();

    ConfigManager cm2(dir.path());
    QCOMPARE(cm2.viewMode(), QStringLiteral("map"));    // persisted

    cm2.setViewMode(QStringLiteral("globe"));
    QCOMPARE(cm2.viewMode(), QStringLiteral("globe"));
    // invalid value clamps to globe
    cm2.setViewMode(QStringLiteral("nonsense"));
    QCOMPARE(cm2.viewMode(), QStringLiteral("globe"));
}
```
Declare `void testViewModeRoundTrip();` under `private slots:`.

- [ ] **Step 2: Run, confirm it fails** (no `viewMode`/`setViewMode` yet)

```bash
cmake --build F:\globe\build --target test_configmanager 2>&1 | Select-Object -Last 4
```
Expected: compile error — no `viewMode()`/`setViewMode()`.

- [ ] **Step 3: Implement in `ConfigManager.h`**

Add to the public getter/setter block (near `alwaysOnTop`):
```cpp
    QString viewMode() const;    void setViewMode(const QString &v);
```
Add a private member (near `m_language`):
```cpp
    QString m_viewMode = QStringLiteral("globe");   // "globe" (3D) or "map" (flat equirectangular)
```

- [ ] **Step 4: Implement in `ConfigManager.cpp`**

Getter/setter:
```cpp
QString ConfigManager::viewMode() const { return m_viewMode; }
void ConfigManager::setViewMode(const QString &v) {
    m_viewMode = (v == QStringLiteral("map")) ? QStringLiteral("map") : QStringLiteral("globe");
}
```
In `load()`, after the `alwaysOnTop` line:
```cpp
    m_viewMode = (obj.value("viewMode").toString(m_viewMode) == QStringLiteral("map"))
                     ? QStringLiteral("map") : QStringLiteral("globe");
```
In `save()`, in the `QJsonObject o` block:
```cpp
    o["viewMode"] = m_viewMode;
```

- [ ] **Step 5: Run, confirm pass**

```bash
cmake --build F:\globe\build --target test_configmanager
cd F:\globe\build; ctest -R test_configmanager --output-on-failure
```
Expected: PASS.

- [ ] **Step 6: Commit**
```bash
git add -A
git commit -m "feat(config): add viewMode setting (globe/map)"
```

---

## Task 2: `TerminatorModel` GL-free helper (TDD)

A pure helper that computes day/night from a sun direction and a lat/lon, shared conceptually with the shader. Pins the terminator math independent of GL.

**Files:**
- Create: `src/celestial/TerminatorModel.h`
- Create: `src/celestial/TerminatorModel.cpp`
- Create: `tests/test_terminator.cpp`
- Modify: `tests/CMakeLists.txt`
- Modify: `CMakeLists.txt` (add the two files to the `celestial` library)

- [ ] **Step 1: Write `src/celestial/TerminatorModel.h`**

```cpp
#pragma once
#include <QVector3D>

// GL-free day/night terminator model. Given the ECEF unit sun direction and a
// surface point by latitude/longitude, returns an illumination factor in
// [0,1]: 1 = full day, 0 = full night, ~0.5 near the terminator. This is the
// CPU-side twin of map.frag's per-pixel math; keeping them in sync guarantees
// the flat-map terminator is unit-testable.
class TerminatorModel {
public:
    // sunDir: ECEF unit vector (e.g. SunModel::sunDirection()).
    // latDeg/lonDeg: surface point in degrees.
    static float dayFactor(QVector3D sunDir, double latDeg, double lonDeg);

    // ECEF unit vector at a lat/lon (degrees).
    static QVector3D surfaceNormal(double latDeg, double lonDeg);

    // Same smoothstep band as the shader (matches earth.frag/map.frag).
    static constexpr double kTerminatorLow = -0.10;
    static constexpr double kTerminatorHigh = 0.20;
};
```

- [ ] **Step 2: Write `src/celestial/TerminatorModel.cpp`**

```cpp
#include "TerminatorModel.h"
#include <cmath>

QVector3D TerminatorModel::surfaceNormal(double latDeg, double lonDeg) {
    const double la = qDegreesToRadians(latDeg);
    const double lo = qDegreesToRadians(lonDeg);
    const double cl = std::cos(la);
    return QVector3D(float(cl * std::cos(lo)),
                     float(cl * std::sin(lo)),
                     float(std::sin(la)));
}

float TerminatorModel::dayFactor(QVector3D sunDir, double latDeg, double lonDeg) {
    const QVector3D p = surfaceNormal(latDeg, lonDeg).normalized();
    const double cosGeo = double(QVector3D::dotProduct(p, sunDir.normalized()));
    // mirror map.frag: smoothstep(-0.10, 0.20, cosGeo)
    double t = (cosGeo - kTerminatorLow) / (kTerminatorHigh - kTerminatorLow);
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    return float(t * t * (3.0 - 2.0 * t));  // smoothstep
}
```

- [ ] **Step 3: Write failing test `tests/test_terminator.cpp`**

```cpp
#include <QtTest/QtTest>
#include "celestial/TerminatorModel.h"

class TestTerminator : public QObject {
    Q_OBJECT
private slots:
    void subSolarPointIsFullDay();
    void antipodeIsFullNight();
    void orthogonalIsMid();
};

void TestTerminator::subSolarPointIsFullDay() {
    // sun overhead at (lat 0, lon 0) -> that point is full day
    QVector3D sun = TerminatorModel::surfaceNormal(0.0, 0.0);
    float f = TerminatorModel::dayFactor(sun, 0.0, 0.0);
    QVERIFY(f > 0.99f);
}

void TestTerminator::antipodeIsFullNight() {
    QVector3D sun = TerminatorModel::surfaceNormal(0.0, 0.0);   // sun over (0,0)
    float f = TerminatorModel::dayFactor(sun, 0.0, 180.0);      // opposite side
    QVERIFY(f < 0.01f);
}

void TestTerminator::orthogonalIsMid() {
    // sun over (0,0); a point 90 degrees away on the equator is near the terminator
    QVector3D sun = TerminatorModel::surfaceNormal(0.0, 0.0);
    float f = TerminatorModel::dayFactor(sun, 0.0, 90.0);
    QVERIFY(f > 0.2f && f < 0.8f);
}

QTEST_MAIN(TestTerminator)
#include "test_terminator.moc"
```

- [ ] **Step 4: Register the test and add the helper to the `celestial` library**

In `tests/CMakeLists.txt`:
```cmake
globe_add_test(test_terminator)
```

In the root `CMakeLists.txt`, inside `qt_add_library(celestial STATIC ...)`, add:
```cmake
    src/celestial/TerminatorModel.h
    src/celestial/TerminatorModel.cpp
```

- [ ] **Step 5: Build + run, confirm pass**

```bash
cmake --build F:\globe\build --target test_terminator
cd F:\globe\build; ctest -R test_terminator --output-on-failure
```
Expected: PASS.

- [ ] **Step 6: Commit**
```bash
git add -A
git commit -m "feat: TerminatorModel GL-free day/night helper + tests"
```

---

## Task 3: `map.vert` / `map.frag` shaders

**Files:**
- Create: `src/celestial/shaders/map.vert`
- Create: `src/celestial/shaders/map.frag`

- [ ] **Step 1: Write `src/celestial/shaders/map.vert`**

A fullscreen quad (2 triangles). Attribute `aPos` in clip-space `[-1,1]`, `aUv` in `[0,1]`:

```glsl
#version 330 core
// Flat equirectangular map: a fullscreen quad. UV maps directly to lon/lat
// because the day/night textures ARE equirectangular 2:1 maps.
layout(location = 0) in vec2 aPos;   // clip-space [-1,1]
layout(location = 1) in vec2 aUv;    // [0,1]
out vec2 vUv;
void main() {
    vUv = aUv;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
```

- [ ] **Step 2: Write `src/celestial/shaders/map.frag`**

```glsl
#version 330 core
// Flat "unwound globe" map: samples the equirectangular day/night textures by
// UV and computes day/night from the TRUE ECEF sun direction (no spin) — the
// faithful real-world terminator. No brown dusk tint (clean transition).
in vec2 vUv;
out vec4 FragColor;

uniform sampler2D uDay;
uniform sampler2D uNight;
uniform vec3  uSunDir;            // ECEF unit sun (real wall-clock sun)
uniform float uHasDay;
uniform float uHasNight;
uniform float uUseNightTexture;   // 1 = city-lights night texture, 0 = dimmed day
uniform float uShowGrid;
uniform float uHasHome;
uniform vec3  uHomeDir;           // ECEF unit vector of the home location
uniform float uTime;

const float PI  = 3.14159265358979;
const float HPI = 1.57079632679490;

const float kGridStepRad = PI / 12.0;      // 15 degrees
const float kGridLinePx  = 1.2;
const vec3  kGridColor   = vec3(1.0, 0.70, 0.28);   // warm amber (matches the globe grid)
const float kGridAlpha   = 0.22;

float gridLine(float coord) {
    float d = abs(fract(coord / kGridStepRad + 0.5) - 0.5) * kGridStepRad;
    float w = fwidth(coord) * kGridLinePx;
    return 1.0 - smoothstep(0.0, w, d);
}

void main() {
    vec2 uv = vUv;
    float lon = (uv.x * 2.0 - 1.0) * PI;    // [-PI, PI]
    float lat = (uv.y * 2.0 - 1.0) * HPI;   // [-PI/2, PI/2]

    float cl = cos(lat);
    vec3 p   = vec3(cl * cos(lon), cl * sin(lon), sin(lat));  // ECEF surface normal
    vec3 sun = normalize(uSunDir);

    float cosGeo   = dot(p, sun);
    float dayFactor = smoothstep(-0.10, 0.20, cosGeo);   // same band as earth.frag

    vec3 day   = (uHasDay   > 0.5) ? texture(uDay,   uv).rgb : vec3(0.15, 0.35, 0.70);
    vec3 night = (uHasNight > 0.5 && uUseNightTexture > 0.5) ? texture(uNight, uv).rgb : vec3(0.0);
    vec3 darkSide = night * 0.8 + day * 0.35 + vec3(0.03, 0.035, 0.05);
    vec3 color = mix(darkSide, day, dayFactor);
    // NOTE: no brown dusk tint here (deliberately clean).

    // Grid: straight lon/lat lines every 15 degrees.
    if (uShowGrid > 0.5) {
        float grid = max(gridLine(lat), gridLine(lon));
        color = mix(color, kGridColor, grid * kGridAlpha);
    }

    // Home marker: angular distance on the sphere to the home point (glued to
    // the surface, same look as the globe beacon).
    if (uHasHome > 0.5) {
        float dd = acos(clamp(dot(p, normalize(uHomeDir)), -1.0, 1.0));
        float pulse = 0.5 + 0.5 * sin(uTime * 2.5);
        vec3  warmOuter = vec3(1.0, 0.50, 0.10);
        vec3  warmInner = vec3(1.0, 0.82, 0.30);
        float glowR = 0.045 + 0.015 * pulse;
        float glow  = exp(-3.0 * (dd / glowR) * (dd / glowR));
        color += warmOuter * glow * (0.40 + 0.30 * pulse);
        float ring = exp(-((dd - 0.022) / 0.004) * ((dd - 0.022) / 0.004)) * 0.55;
        color += warmInner * ring;
        float core = smoothstep(0.010, 0.004, dd);
        color = mix(color, vec3(1.0), core);
    }

    FragColor = vec4(color, 1.0);
}
```

- [ ] **Step 3: Commit** (shaders load at runtime; no build step needed to "compile" them)

```bash
git add src/celestial/shaders/map.vert src/celestial/shaders/map.frag
git commit -m "feat(shaders): flat equirectangular map shader (day/night + grid + marker)"
```

---

## Task 4: `CelestialBody::renderMap()` + quad geometry

**Files:**
- Modify: `src/celestial/CelestialBody.h`
- Modify: `src/celestial/CelestialBody.cpp`

- [ ] **Step 1: Extend `CelestialBody.h`**

Add a public method near `render()`:
```cpp
    void renderMap();   // flat equirectangular map (flat-map mode)
```
Add private members near the sphere buffers:
```cpp
    std::unique_ptr<QOpenGLShaderProgram> m_mapProg;
    GLuint m_quadVao = 0, m_quadVbo = 0, m_quadIbo = 0;
    void buildQuad();
```

- [ ] **Step 2: In `CelestialBody.cpp`, compile the map program + build the quad in `initialize()`**

Inside `initialize()` (after the existing `buildSphere(...)` line, before `if (m_assets) loadTextures();`), add:
```cpp
    m_mapProg = makeProgram("map.vert", "map.frag");
    buildQuad();
```

- [ ] **Step 3: Implement `buildQuad()`** (a 4-vert / 6-index fullscreen quad; each vert has `vec2 aPos, vec2 aUv`)

```cpp
void CelestialBody::buildQuad() {
    // 4 corners: (clip-space pos xy, uv xy)
    // bottom-left, bottom-right, top-right, top-left
    const float verts[] = {
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
        -1.0f,  1.0f,  0.0f, 1.0f,
    };
    const GLuint idx[] = { 0, 1, 2, 0, 2, 3 };

    m_gl->glGenVertexArrays(1, &m_quadVao);
    m_gl->glGenBuffers(1, &m_quadVbo);
    m_gl->glGenBuffers(1, &m_quadIbo);
    m_gl->glBindVertexArray(m_quadVao);
    m_gl->glBindBuffer(GL_ARRAY_BUFFER, m_quadVbo);
    m_gl->glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    m_gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_quadIbo);
    m_gl->glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
    m_gl->glEnableVertexAttribArray(0);
    m_gl->glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    m_gl->glEnableVertexAttribArray(1);
    m_gl->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void *>(2 * sizeof(float)));
    m_gl->glBindVertexArray(0);
}
```

- [ ] **Step 4: Implement `renderMap()`**

```cpp
void CelestialBody::renderMap() {
    if (!m_gl) return;
    m_gl->glClearColor(0.02f, 0.03f, 0.05f, 1.0f);
    m_gl->glDisable(GL_DEPTH_TEST);
    m_gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const QVector3D sun = m_sun ? m_sun->sunDirection() : QVector3D(1, 0, 0);

    m_mapProg->bind();
    m_mapProg->setUniformValue("uSunDir", sun);
    m_mapProg->setUniformValue("uHasDay", m_texDay ? 1.0f : 0.0f);
    m_mapProg->setUniformValue("uHasNight", m_texNight ? 1.0f : 0.0f);
    m_mapProg->setUniformValue("uUseNightTexture", m_opts.useNightTexture ? 1.0f : 0.0f);
    m_mapProg->setUniformValue("uShowGrid", m_opts.showGrid ? 1.0f : 0.0f);
    m_mapProg->setUniformValue("uHasHome", m_opts.hasHome ? 1.0f : 0.0f);
    {
        const double la = qDegreesToRadians(m_opts.homeLat);
        const double lo = qDegreesToRadians(m_opts.homeLon);
        const QVector3D homeDir(float(std::cos(la) * std::cos(lo)),
                                float(std::cos(la) * std::sin(lo)),
                                float(std::sin(la)));
        m_mapProg->setUniformValue("uHomeDir", homeDir);
    }
    m_mapProg->setUniformValue("uTime", float(std::fmod(QDateTime::currentMSecsSinceEpoch() / 1000.0, 600.0)));
    if (m_texDay)   { m_texDay->bind(0);     m_mapProg->setUniformValue("uDay", 0); }
    if (m_texNight) { m_texNight->bind(1);   m_mapProg->setUniformValue("uNight", 1); }

    m_gl->glBindVertexArray(m_quadVao);
    m_gl->glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    m_gl->glBindVertexArray(0);
}
```

- [ ] **Step 5: Build the library**

```bash
cmake --build F:\globe\build --target celestial 2>&1 | Select-Object -Last 4
```
Expected: compiles. (`QDateTime`/`cmath`/`qDegreesToRadians` are already included at the top of `CelestialBody.cpp`.)

- [ ] **Step 6: Commit**
```bash
git add -A
git commit -m "feat(render): CelestialBody flat-map render path (renderMap + quad)"
```

---

## Task 5: `CelestialWidget::ViewMode` + toggle/reshape

**Files:**
- Modify: `src/celestial/CelestialWidget.h`
- Modify: `src/celestial/CelestialWidget.cpp`

- [ ] **Step 1: Extend `CelestialWidget.h`**

Add a public enum + methods (near `setHomeLocation`):
```cpp
    enum class ViewMode { Globe, FlatMap };
    void setViewMode(ViewMode m);
    ViewMode viewMode() const { return m_viewMode; }
```
Add a private member (after `m_hasLocationFix`):
```cpp
    ViewMode m_viewMode = ViewMode::Globe;
```

- [ ] **Step 2: In `CelestialWidget.cpp`, branch `paintGL()`**

Replace the body of `paintGL()`:
```cpp
void CelestialWidget::paintGL() {
    if (m_viewMode == ViewMode::FlatMap)
        m_body.renderMap();
    else
        m_body.render();
}
```

- [ ] **Step 3: Branch `resizeEvent()` — elliptical mask only in Globe mode**

In `resizeEvent()`, wrap the existing mask code in a mode check:
```cpp
void CelestialWidget::resizeEvent(QResizeEvent *e) {
    QOpenGLWidget::resizeEvent(e);
    if (m_viewMode == ViewMode::Globe) {
        const int side = qMin(width(), height());
        const QRect r((width() - side) / 2, (height() - side) / 2, side, side);
        setMask(QRegion(r, QRegion::Ellipse));
    } else {
        // Flat map: rectangular widget, no mask.
        clearMask();
    }
}
```

- [ ] **Step 4: Implement `setViewMode()`**

```cpp
void CelestialWidget::setViewMode(ViewMode m) {
    if (m_viewMode == m) return;
    m_viewMode = m;
    const int w = width();
    const QPoint c = geometry().center();
    if (m == ViewMode::FlatMap) {
        const int newH = qMax(120, w / 2);            // 2:1 keeping the width
        resize(w, newH);
        move(c - QPoint(w / 2, newH / 2));
        clearMask();
    } else {
        resize(w, w);                                  // square again
        move(c - QPoint(w / 2, w / 2));
        // elliptical mask re-applied by resizeEvent()
    }
    update();
}
```

- [ ] **Step 5: Branch `wheelEvent()` — keep 2:1 in FlatMap**

In `wheelEvent()`, branch on mode. Replace the resize computation:
```cpp
void CelestialWidget::wheelEvent(QWheelEvent *e) {
    const int delta = e->angleDelta().y();
    const QPoint center = geometry().center();
    if (m_viewMode == ViewMode::FlatMap) {
        // Keep 2:1: change width, height = width/2.
        const int newW = qBound(240, int(width() + delta * 0.3), 1024);  // >=240 so height>=120
        if (newW == width()) { QOpenGLWidget::wheelEvent(e); return; }
        const int newH = qMax(120, newW / 2);
        resize(newW, newH);
        move(center - QPoint(newW / 2, newH / 2));
    } else {
        const int newSide = qBound(160, int(width() + delta * 0.3), 1024);
        if (newSide == width()) { QOpenGLWidget::wheelEvent(e); return; }
        resize(newSide, newSide);
        move(center - QPoint(newSide / 2, newSide / 2));
    }
    if (m_config) { m_config->setDiameter(width()); m_config->save(); }
    emit userInteracted();
    update();
    QOpenGLWidget::wheelEvent(e);
}
```
(Match the existing `wheelEvent` body for the non-mode-specific parts; the key change is the FlatMap branch + emitting `userInteracted()`.)

- [ ] **Step 6: Make camera controls no-ops in FlatMap**

In `mouseMoveEvent()`, the camera-rotate branch (the `else if ((e->buttons() & Qt::LeftButton) && m_cam)`) must be skipped in FlatMap — wrap it:
```cpp
    } else if (m_viewMode == ViewMode::Globe && (e->buttons() & Qt::LeftButton) && m_cam) {
        // ... existing camera drag ...
    }
```
In `mouseDoubleClickEvent()`, early-return in FlatMap (nothing to reset):
```cpp
void CelestialWidget::mouseDoubleClickEvent(QMouseEvent *e) {
    if (m_viewMode == ViewMode::FlatMap) { QOpenGLWidget::mouseDoubleClickEvent(e); return; }
    // ... existing globe reset ...
}
```

- [ ] **Step 7: Build**
```bash
cmake --build F:\globe\build --target celestial 2>&1 | Select-Object -Last 4
```
Expected: compiles.

- [ ] **Step 8: Commit**
```bash
git add -A
git commit -m "feat(widget): ViewMode toggle — globe <-> flat map (mask + 2:1 reshape)"
```

---

## Task 6: `SystemTray` "Flat Map" checkable action

**Files:**
- Modify: `src/celestial/SystemTray.h`
- Modify: `src/celestial/SystemTray.cpp`

- [ ] **Step 1: Extend `SystemTray.h`**

Add a signal + a setter (the setter lets `main.cpp` sync the checkbox to the persisted state on startup without re-triggering the toggle):
```cpp
signals:
    void flatMapToggled(bool flat);
public:
    void setFlatMapChecked(bool checked);
private:
    QAction *m_flatMapAction = nullptr;
```
(Add `#include <QAction>` forward decl is already there as `class QAction;` — but now we hold a pointer as a member, the forward decl suffices; keep it.)

- [ ] **Step 2: Add the checkable action in `SystemTray.cpp` ctor**

After the `Center on Me` line and before the `addSeparator()` before Settings:
```cpp
    m_flatMapAction = menu->addAction(tr("Flat Map"));
    m_flatMapAction->setCheckable(true);
    connect(m_flatMapAction, &QAction::toggled, this, &SystemTray::flatMapToggled);
```

- [ ] **Step 3: Implement `setFlatMapChecked()`** (block signals so it doesn't re-emit)

```cpp
void SystemTray::setFlatMapChecked(bool checked) {
    if (m_flatMapAction) {
        QSignalBlocker b(m_flatMapAction);
        m_flatMapAction->setChecked(checked);
    }
}
```
(Add `#include <QSignalBlocker>` at the top of `SystemTray.cpp`.)

- [ ] **Step 4: Build + commit**
```bash
cmake --build F:\globe\build --target celestial 2>&1 | Select-Object -Last 3
git add -A
git commit -m "feat(tray): checkable 'Flat Map' action + flatMapToggled signal"
```

---

## Task 7: Wire the toggle in `main.cpp`

**Files:**
- Modify: `src/apps/earth/main.cpp`

- [ ] **Step 1: Apply persisted mode on startup + connect the tray toggle**

After the `SystemTray tray;` / `SettingsDialog settingsDialog(...)` block and the existing tray connections, add:

```cpp
    // --- Flat map mode toggle (globe <-> equirectangular map) ---
    QObject::connect(&tray, &SystemTray::flatMapToggled, &widget,
        [&widget, &config](bool flat) {
            widget.setViewMode(flat ? CelestialWidget::ViewMode::FlatMap
                                    : CelestialWidget::ViewMode::Globe);
            config.setViewMode(flat ? QStringLiteral("map") : QStringLiteral("globe"));
            config.save();
        });
    const bool startFlat = (config.viewMode() == QStringLiteral("map"));
    tray.setFlatMapChecked(startFlat);
    if (startFlat)
        widget.setViewMode(CelestialWidget::ViewMode::FlatMap);
```

- [ ] **Step 2: Make Reset View / Center on Me no-ops in flat-map mode**

In the `resetView` and `centerOnMe` lambdas, add an early guard:
```cpp
    QObject::connect(&tray, &SystemTray::resetView, &widget, [&widget, &camera] {
        if (widget.viewMode() != CelestialWidget::ViewMode::Globe) return;
        camera.reset();
        // ... rest unchanged ...
    });
    QObject::connect(&tray, &SystemTray::centerOnMe, &widget, [&widget, &camera, &config] {
        if (widget.viewMode() != CelestialWidget::ViewMode::Globe) return;
        camera.reset();
        // ... rest unchanged ...
    });
```

- [ ] **Step 3: Build the full app + run all tests**
```bash
cmake --build F:\globe\build 2>&1 | Select-Object -Last 4
cd F:\globe\build; ctest --output-on-failure
```
Expected: builds; all tests pass (including the new `test_terminator` and the `viewMode` cases).

- [ ] **Step 4: Commit**
```bash
git add -A
git commit -m "feat(earth): wire flat-map toggle (tray + persisted startup mode)"
```

---

## Task 8: Verification

- [ ] **Step 1: Launch and toggle**
```bash
Stop any running earth; Start-Process F:\globe\build\earth.exe
```
Right-click tray → check **Flat Map**:
- Widget reshapes to a 2:1 rectangle showing the whole world.
- Day side lit, night side dark (city lights if Night Mode = Realistic), **soft terminator** down the middle.
- Grid (if enabled) shows as straight lon/lat lines.
- The terminator drifts over time (real-world sun).

Uncheck **Flat Map** → returns to the circular spinning globe.

- [ ] **Step 2: Persistence**
Quit, relaunch → opens in the mode you last selected.

- [ ] **Step 3: Resize sanity**
In Flat Map, wheel-resize keeps 2:1. In Globe, wheel-resize stays square.

- [ ] **Step 4: Commit any final touch-ups; tag**
```bash
git tag flat-map-mode
```

---

## Spec Coverage

| Spec requirement | Task |
|------------------|------|
| `viewMode` config (globe/map, default globe) | Task 1 |
| GL-free terminator math, unit-tested | Task 2 |
| `map.vert`/`map.frag` (flat sampling, true sun, grid, marker, no dusk tint) | Task 3 |
| `CelestialBody::renderMap()` + quad | Task 4 |
| Widget `ViewMode` toggle + mask/2:1 reshape + camera no-ops | Task 5 |
| Tray "Flat Map" checkable action | Task 6 |
| main.cpp wiring + persisted startup + reset/center guards | Task 7 |
| Live real-time terminator (no spin) | Tasks 3–4 (uses existing SunModel/TimeController, no extra wiring) |
| Grid + marker carry over | Tasks 3–4 (read existing `m_opts`) |
| Verification | Task 8 |

## Self-Review Notes

- **Type consistency:** `ViewMode::Globe`/`FlatMap` used identically in widget + main. `viewMode()` returns `"globe"`/`"map"` strings consistently in config + main. `flatMapToggled(bool)` / `setFlatMapChecked(bool)` pair correctly.
- **No placeholders:** every code step shows full code.
- **Earth path untouched:** Tasks 4–5 only ADD `renderMap()`/`buildQuad()`/mode branches; the existing `render()`/sphere path is unchanged.
