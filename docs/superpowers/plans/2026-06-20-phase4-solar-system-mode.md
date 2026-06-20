# Phase 4 — Solar System Mode Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: superpowers:subagent-driven-development or superpowers:executing-plans. Steps use checkbox (`- [ ]`) syntax.

**Goal:** Add the unified **Solar System Mode** to `solar.exe` — a large window rendering the Sun at the center, all planets orbiting it in 3D with correct relative motion, a Milky-Way starfield background, orbit trails, a time-scale slider, and free camera control (drag-orbit, scroll-zoom).

**Architecture:** A new `OrbitModel` computes heliocentric positions from the Keplerian elements already stored in `BodyConfig`. A new `SolarSystemScene` renders, in one OpenGL context: a skybox, faint orbit ellipses, every body (`CelestialBody` reused per-body, but positioned at its orbit and scaled), and a HUD overlay (simulated date + time scale). `solar.exe` gains a "Solar System Mode" button that shows this scene window.

**Tech Stack:** Qt6, OpenGL 3.3 Core, existing celestial library + shaders, new skybox/orbit shaders.

**Key design decision — distance/size scaling:** Real AU and km ratios make the outer planets invisible and far off-screen. Use a **logarithmic distance scale** (so Neptune stays on-screen) and a **visual radius scale** that exaggerates planet sizes relative to the Sun. The Sun is rendered at a capped visual size so it doesn't swallow everything.

---

## File Structure

```
src/celestial/
├── OrbitModel.h/.cpp            (NEW — Keplerian solver)
├── SolarSystemScene.h/.cpp      (NEW — unified scene renderer)
├── shaders/
│   ├── skybox.vert/frag         (NEW)
│   └── orbit.vert/frag          (NEW)
└── (existing)
src/apps/solar/
├── LauncherWindow.h/.cpp        (modified — add "Solar System Mode" button)
├── SolarSystemWindow.h/.cpp     (NEW — QOpenGLWidget host + camera + HUD + slider)
tests/
└── test_orbitmodel.cpp          (NEW)
```

---

## Task 1: `OrbitModel` — Keplerian position solver

**Files:**
- Create: `src/celestial/OrbitModel.h`
- Create: `src/celestial/OrbitModel.cpp`
- Create: `tests/test_orbitmodel.cpp`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: `OrbitModel.h`**

```cpp
#pragma once
#include "BodyConfig.h"
#include <QVector3D>
#include <QDateTime>

// Computes heliocentric position from J2000-mean Keplerian elements stored in
// BodyConfig, advanced by a simulated time scaled from real time. GL-free, so
// it is unit-testable.
class OrbitModel {
public:
    explicit OrbitModel(const BodyConfig& c);

    // Advance simulated time by dtSeconds * timeScale().
    void update(float dtSeconds);
    void setTimeScale(float yearsPerMinute);
    float timeScale() const { return m_yearsPerMinute; }

    // Heliocentric position in AU (orbital plane coordinates).
    QVector3D positionAU() const;
    // Simulated UTC date.
    QDateTime simulatedDateTime() const { return m_simTime; }

    static constexpr float kDefaultYearsPerMinute = 1.0f;  // 1 year = 60s
private:
    BodyConfig m_config;
    QDateTime m_simTime = QDateTime::currentDateTimeUtc();
    float m_yearsPerMinute = kDefaultYearsPerMinute;

    // Solve Kepler's equation M = E - e*sinE for E (eccentric anomaly) by Newton-Raphson.
    static double solveKepler(double M_rad, double e);
    // Convert orbital elements -> cartesian AU at given time.
    QVector3D computePosition(const QDateTime& t) const;
};
```

- [ ] **Step 2: `OrbitModel.cpp`** — implement:
- Julian centuries since J2000 from `m_simTime`.
- Mean anomaly at time = `meanAnomalyAtEpochDeg + n * dt` where n = mean motion (deg/day) from `semiMajorAxisAU` via Kepler's 3rd law.
- `solveKepler` (Newton-Raphson, ~6 iterations, e<1).
- Convert E → true anomaly ν → heliocentric distance r = a(1−e·cosE) → position in orbital plane → rotate by ω (arg of perihelion), Ω (long. asc. node), i (inclination).

- [ ] **Step 3: `test_orbitmodel.cpp`**

```cpp
void testEarthDistanceNearOneAU() {
    OrbitModel o(BodyConfig::earth()... /* or a planet with known a */);
    // Use mars: a=1.524 AU; at any time r should be within [a(1-e), a(1+e)]
    OrbitModel m(BodyConfig::mars());
    float a = BodyConfig::mars().semiMajorAxisAU;
    float e = BodyConfig::mars().eccentricity;
    float r = m.positionAU().length();
    QVERIFY(r >= a*(1-e)*0.99f && r <= a*(1+e)*1.01f);
}
void testTimeScaleAdvancesTime() {
    OrbitModel o(BodyConfig::mars());
    QDateTime before = o.simulatedDateTime();
    o.setTimeScale(1.0f);
    o.update(60.0f);  // 1 year per minute -> 1 year advanced
    QVERIFY(o.simulatedDateTime() > before.addMonths(6));
}
```

- [ ] **Step 4: Add test, build, run, commit**

```bash
cmake --build F:\globe\build --target test_orbitmodel
cd F:\globe\build && ctest -R test_orbitmodel --output-on-failure
git add -A && git commit -m "feat: OrbitModel Keplerian position solver + tests"
```

---

## Task 2: Skybox + orbit shaders

**Files:**
- Create: `src/celestial/shaders/skybox.vert`
- Create: `src/celestial/shaders/skybox.frag`
- Create: `src/celestial/shaders/orbit.vert`
- Create: `src/celestial/shaders/orbit.frag`

- [ ] **skybox.vert/frag**: render a full-screen triangle (or cube) sampling the Milky Way texture as an equirectangular backdrop. For simplicity, sample `8k_stars_milky_way.jpg` as a 2D background textured by screen direction (use the inverse view-rotation so the starfield follows camera yaw/pitch).

- [ ] **orbit.vert/frag**: `GL_LINE_LOOP` of 128 segments; `uniform vec4 uColor` (faint amber); no lighting.

- [ ] Commit: `git commit -m "feat: skybox and orbit shaders"`

---

## Task 3: `SolarSystemScene` — unified renderer

**Files:**
- Create: `src/celestial/SolarSystemScene.h`
- Create: `src/celestial/SolarSystemScene.cpp`

- [ ] **Step 1: Interface**

```cpp
class SolarSystemScene {
public:
    void initialize(QOpenGLFunctions_3_3_Core* gl, AssetManager* assets);
    void resize(int w, int h, float dpr);
    void update(float dtSeconds);   // advances all OrbitModels + body spins
    void render(const QMatrix4x4& view, const QMatrix4x4& proj, const QVector3D& camPos);

    void setShowOrbits(bool b); void setShowLabels(bool b); void setShowMilkyWay(bool b);
    void setTimeScale(float ypm);
    QDateTime simulatedDateTime() const;
private:
    struct Body { BodyConfig config; CelestialBody renderer; OrbitModel orbit; };
    std::vector<Body> m_bodies;
    // ... skybox texture, orbit VBOs, label font ...
    float distanceScale(float au) const;  // log-compressed
    float radiusScale(float km) const;    // exaggerated, sun capped
};
```

- [ ] **Step 2: Populate bodies** from all `BodyConfig` factories. Sun at origin (no orbit). Build a `GL_LINE_LOOP` per planet for orbit trails using the orbit's ellipse (sample 128 points).

- [ ] **Step 3: `render` order**: skybox → orbits → bodies (each at `distanceScale(au)` position, scaled by `radiusScale(km)`, axial tilt applied). Sun uses emissive path.

- [ ] **Step 4: Commit**: `git commit -m "feat: SolarSystemScene unified renderer"`

---

## Task 4: `SolarSystemWindow` — host + camera + HUD + slider

**Files:**
- Create: `src/apps/solar/SolarSystemWindow.h`
- Create: `src/apps/solar/SolarSystemWindow.cpp`

- [ ] **Step 1: A borderless `QOpenGLWidget`** (or framed) sized large (e.g. 900×900). Hosts `SolarSystemScene`.

- [ ] **Step 2: Camera**: spherical orbit camera (yaw/pitch/distance). Mouse drag = yaw/pitch, wheel = zoom. Build view matrix from spherical coords.

- [ ] **Step 3: HUD overlay** (paint with `QPainter` over the GL widget in `paintGL` after `scene.render`): simulated date (top-left), time-scale value (bottom), a horizontal `QSlider` bound to `setTimeScale` (bottom). Toggles (orbits/labels/Milky Way) via a small menu or checkboxes.

- [ ] **Step 4: Frame timer** at fps cap calling `scene.update(dt)` then `update()`.

- [ ] **Step 5: Commit**: `git commit -m "feat(solar): SolarSystemWindow with camera + HUD + time slider"`

---

## Task 5: Wire "Solar System Mode" into launcher; build; verify

**Files:**
- Modify: `src/apps/solar/LauncherWindow.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add button** "Solar System Mode" to the launcher that shows `SolarSystemWindow` (modal-less; hide launcher while open, show on close).

- [ ] **Step 2: Add the two new source files** to the `solar` CMake target.

- [ ] **Step 3: Build + run**

```bash
cmake --build F:\globe\build --target solar
F:\globe\build\solar.exe
```

Click "Solar System Mode" → verify: starfield background, Sun at center, planets orbiting, time slider advances date, drag rotates view, wheel zooms.

- [ ] **Step 4: Commit + tag**

```bash
git add -A && git commit -m "feat(solar): Solar System Mode entry point"
git tag phase4-solar-system-mode
```

---

## Spec Coverage

| Spec item | Task |
|-----------|------|
| OrbitModel (Keplerian) | Task 1 |
| Skybox / Milky Way | Task 2 |
| SolarSystemScene | Task 3 |
| Camera + time scale + HUD | Task 4 |
| solar.exe Solar System Mode | Task 5 |
| Tests | Task 1 |
