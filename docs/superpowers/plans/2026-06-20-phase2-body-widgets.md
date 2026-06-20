# Phase 2 — Standalone Body Widgets Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Produce standalone desktop widgets for Sun, Moon, and all 7 non-Earth planets (Mercury, Venus, Mars, Jupiter, Saturn, Uranus, Neptune), each its own executable (`sun.exe`, `moon.exe`, …), reusing the `celestial` library from Phase 1.

**Architecture:** Generalize `AssetManager` to load textures by filename (driven by `BodyConfig`) instead of the Earth-specific `Slot` enum. Add an emissive render path for the Sun and a ring render path for Saturn. Each body = a `BodyConfig` factory + a thin `main.cpp` + a CMake target. Introduce per-body config directories so each body persists its own settings.

**Tech Stack:** Qt6, OpenGL 3.3 Core, CMake, existing GLSL pipeline in `src/celestial/shaders/`.

---

## File Structure After Phase 2

```
src/
├── celestial/
│   ├── AssetManager.h/.cpp        (modified: add filename-based image())
│   ├── BodyConfig.h/.cpp          (modified: add factories for all bodies + orbital fields)
│   ├── CelestialBody.h/.cpp       (modified: body-type shader selection, ring pass)
│   ├── ConfigManager.h/.cpp       (modified: per-body config dir)
│   ├── shaders/
│   │   ├── earth.vert/frag        (existing — rocky/gas planet)
│   │   ├── star.vert/frag         (NEW — emissive Sun)
│   │   └── ring.vert/frag         (NEW — Saturn rings)
│   └── ... (unchanged from Phase 1)
├── apps/
│   ├── earth/main.cpp             (existing)
│   ├── sun/main.cpp               (NEW)
│   ├── moon/main.cpp              (NEW)
│   ├── mercury/main.cpp           (NEW)
│   ├── venus/main.cpp             (NEW)
│   ├── mars/main.cpp              (NEW)
│   ├── jupiter/main.cpp           (NEW)
│   ├── saturn/main.cpp            (NEW)
│   ├── uranus/main.cpp            (NEW)
│   └── neptune/main.cpp           (NEW)
└── textures/  (F:\globe\textures — populated with body textures)
tests/
├── test_bodyconfig.cpp            (modified: assert all factories)
└── test_assetmanager.cpp          (NEW — filename-based lookup)
```

## Texture Inventory

Source textures in `C:\Users\huang\Downloads\textures`. Copy into `F:\globe\textures` with normalized names:

| Body | Source | Target |
|------|--------|--------|
| Sun | `8k_sun.jpg` | `sun.jpg` |
| Moon | `8k_moon.jpg` | `moon.jpg` |
| Mercury | `8k_mercury.jpg` | `mercury.jpg` |
| Venus | `4k_venus_atmosphere.jpg` | `venus.jpg` |
| Mars | `8k_mars.jpg` | `mars.jpg` |
| Jupiter | `8k_jupiter.jpg` | `jupiter.jpg` |
| Saturn | `8k_saturn.jpg` | `saturn.jpg` |
| Saturn ring | `8k_saturn_ring_alpha.png` | `saturn_ring.png` |
| Uranus | `2k_uranus.jpg` | `uranus.jpg` |
| Neptune | `2k_neptune.jpg` | `neptune.jpg` |

(Earth textures `day.jpg`/`night.jpg`/`normal.png`/`specular.png`/`clouds.png` already in `F:\globe\textures`.)

---

## Task 1: Generalize `AssetManager` to load textures by filename

**Files:**
- Modify: `src/celestial/AssetManager.h`
- Modify: `src/celestial/AssetManager.cpp`
- Create: `tests/test_assetmanager.cpp`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Add filename-based API to `AssetManager.h`**

Add a public method that loads any named file from the search dirs, keeping the existing `Slot` enum for Earth backward-compat:

```cpp
// Load an arbitrary named image from the search dirs. Returns procedural
// fallback (solid color) if not found. tierMaxSize caps the long edge.
QImage image(const QString& fileName, int tierMaxSize) const;
bool hasFile(const QString& fileName) const;
```

(Keep the existing `image(Slot, int)` and `fileName(Slot)`.)

- [ ] **Step 2: Implement filename-based methods in `AssetManager.cpp`**

```cpp
bool AssetManager::hasFile(const QString& fileName) const {
    for (const QString& d : m_dirs) {
        if (QFile::exists(d + QLatin1Char('/') + fileName)) return true;
    }
    return false;
}

QImage AssetManager::image(const QString& fileName, int tierMaxSize) const {
    for (const QString& d : m_dirs) {
        const QString p = d + QLatin1Char('/') + fileName;
        if (QFile::exists(p)) {
            QImage img(p);
            if (!img.isNull()) {
                const int w = img.width(), h = img.height();
                if (w > tierMaxSize) {
                    const int nh = int(qint64(h) * tierMaxSize / qMax(1, w));
                    img = img.scaled(tierMaxSize, nh, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                }
                return img;
            }
        }
    }
    // Procedural fallback: neutral gray so the body is visible, not black.
    QImage fb(512, 256, QImage::Format_RGB32);
    fb.fill(QColor(120, 120, 120));
    return fb;
}
```

- [ ] **Step 3: Write failing test `tests/test_assetmanager.cpp`**

```cpp
#include <QtTest/QtTest>
#include "celestial/AssetManager.h"
#include <QCoreApplication>

class TestAssetManager : public QObject {
    Q_OBJECT
private slots:
    void filenameLookupResolvesExistingFile();
    void filenameLookupFallsBackWhenMissing();
};

void TestAssetManager::filenameLookupResolvesExistingFile() {
    // earth day.jpg ships in F:\globe\textures (../textures from build dir)
    AssetManager am;
    QImage img = am.image(QStringLiteral("day.jpg"), 4096);
    QVERIFY(!img.isNull());
    QVERIFY(img.width() > 0);
}

void TestAssetManager::filenameLookupFallsBackWhenMissing() {
    AssetManager am;
    QImage img = am.image(QStringLiteral("does_not_exist_xyz.jpg"), 4096);
    QVERIFY(!img.isNull());  // procedural fallback, not null
}

QTEST_MAIN(TestAssetManager)
#include "test_assetmanager.moc"
```

- [ ] **Step 4: Add test to `tests/CMakeLists.txt`**

```cmake
globe_add_test(test_assetmanager)
```

- [ ] **Step 5: Build + run test**

```bash
cmake --build F:\globe\build --target test_assetmanager
cd F:\globe\build && ctest -R test_assetmanager --output-on-failure
```

Expected: PASS.

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "feat: AssetManager filename-based texture lookup for multi-body support"
```

---

## Task 2: Populate body textures

**Files:**
- Add: `F:\globe\textures\{sun,moon,mercury,venus,mars,jupiter,saturn,uranus,neptune}.jpg`
- Add: `F:\globe\textures\saturn_ring.png`

- [ ] **Step 1: Copy textures from Downloads**

```bash
$src = "C:\Users\huang\Downloads\textures"
$dst = "F:\globe\textures"
Copy-Item "$src\8k_sun.jpg"            "$dst\sun.jpg"
Copy-Item "$src\8k_moon.jpg"           "$dst\moon.jpg"
Copy-Item "$src\8k_mercury.jpg"        "$dst\mercury.jpg"
Copy-Item "$src\4k_venus_atmosphere.jpg" "$dst\venus.jpg"
Copy-Item "$src\8k_mars.jpg"           "$dst\mars.jpg"
Copy-Item "$src\8k_jupiter.jpg"        "$dst\jupiter.jpg"
Copy-Item "$src\8k_saturn.jpg"         "$dst\saturn.jpg"
Copy-Item "$src\8k_saturn_ring_alpha.png" "$dst\saturn_ring.png"
Copy-Item "$src\2k_uranus.jpg"         "$dst\uranus.jpg"
Copy-Item "$src\2k_neptune.jpg"        "$dst\neptune.jpg"
```

- [ ] **Step 2: Verify all present**

```bash
Get-ChildItem F:\globe\textures | Select-Object Name
```

Expected: sun/moon/mercury/venus/mars/jupiter/saturn/uranus/neptune .jpg + saturn_ring.png + earth textures.

- [ ] **Step 3: Commit**

```bash
git add textures
git commit -m "assets: add solar system body textures"
```

---

## Task 3: Extend `BodyConfig` with all body factories + orbital fields

**Files:**
- Modify: `src/celestial/BodyConfig.h`
- Modify: `src/celestial/BodyConfig.cpp`
- Modify: `tests/test_bodyconfig.cpp`

- [ ] **Step 1: Add orbital + ring fields to `BodyConfig.h`**

```cpp
struct BodyConfig {
    QString bodyId;
    QString displayName;
    QString albedoTexture;
    QString nightTexture;       // optional
    QString normalTexture;      // optional
    QString specularTexture;    // optional
    QString cloudTexture;       // optional
    QString ringTexture;        // optional (Saturn)

    float radiusKm = 6371.0f;
    float visualScale = 1.0f;
    float rotationPeriodHours = 24.0f;
    float axialTiltDegrees = 23.5f;
    bool emissive = false;       // Sun
    bool hasRings = false;       // Saturn
    bool hasClouds = false;
    bool hasAtmosphere = false;

    // Heliocentric orbital elements (planets only; unused for Sun/Moon-Earth)
    float semiMajorAxisAU = 0.0f;
    float eccentricity = 0.0f;
    float inclinationDeg = 0.0f;
    float longitudeAscendingNodeDeg = 0.0f;
    float argumentOfPerihelionDeg = 0.0f;
    float meanAnomalyAtEpochDeg = 0.0f;

    static BodyConfig sun();
    static BodyConfig moon();
    static BodyConfig mercury();
    static BodyConfig venus();
    static BodyConfig earth();
    static BodyConfig mars();
    static BodyConfig jupiter();
    static BodyConfig saturn();
    static BodyConfig uranus();
    static BodyConfig neptune();
};
```

- [ ] **Step 2: Implement factories in `BodyConfig.cpp`**

Use J2000-mean orbital elements (good enough for desktop visuals). Keep `earth()` exactly as-is (existing texture names).

```cpp
BodyConfig BodyConfig::sun() {
    BodyConfig c;
    c.bodyId = QStringLiteral("sun");
    c.displayName = QStringLiteral("Sun");
    c.albedoTexture = QStringLiteral("sun.jpg");
    c.radiusKm = 696340.0f;
    c.rotationPeriodHours = 609.12f;   // ~25.4 days
    c.axialTiltDegrees = 7.25f;
    c.emissive = true;
    return c;
}

BodyConfig BodyConfig::moon() {
    BodyConfig c;
    c.bodyId = QStringLiteral("moon");
    c.displayName = QStringLiteral("Moon");
    c.albedoTexture = QStringLiteral("moon.jpg");
    c.radiusKm = 1737.4f;
    c.rotationPeriodHours = 655.72f;   // tidally locked
    c.axialTiltDegrees = 6.68f;
    return c;
}

BodyConfig BodyConfig::mercury() {
    BodyConfig c;
    c.bodyId = QStringLiteral("mercury"); c.displayName = QStringLiteral("Mercury");
    c.albedoTexture = QStringLiteral("mercury.jpg");
    c.radiusKm = 2439.7f; c.rotationPeriodHours = 1407.6f; c.axialTiltDegrees = 0.03f;
    c.semiMajorAxisAU = 0.387f; c.eccentricity = 0.2056f;
    c.inclinationDeg = 7.0f; c.longitudeAscendingNodeDeg = 48.3f;
    c.argumentOfPerihelionDeg = 29.1f; c.meanAnomalyAtEpochDeg = 174.8f;
    return c;
}

BodyConfig BodyConfig::venus() {
    BodyConfig c;
    c.bodyId = QStringLiteral("venus"); c.displayName = QStringLiteral("Venus");
    c.albedoTexture = QStringLiteral("venus.jpg");
    c.radiusKm = 6051.8f; c.rotationPeriodHours = -5832.5f; c.axialTiltDegrees = 177.4f;  // retrograde
    c.hasAtmosphere = true;
    c.semiMajorAxisAU = 0.723f; c.eccentricity = 0.0068f;
    c.inclinationDeg = 3.4f; c.longitudeAscendingNodeDeg = 76.7f;
    c.argumentOfPerihelionDeg = 54.9f; c.meanAnomalyAtEpochDeg = 50.4f;
    return c;
}

BodyConfig BodyConfig::mars() {
    BodyConfig c;
    c.bodyId = QStringLiteral("mars"); c.displayName = QStringLiteral("Mars");
    c.albedoTexture = QStringLiteral("mars.jpg");
    c.radiusKm = 3389.5f; c.rotationPeriodHours = 24.62f; c.axialTiltDegrees = 25.19f;
    c.semiMajorAxisAU = 1.524f; c.eccentricity = 0.0934f;
    c.inclinationDeg = 1.85f; c.longitudeAscendingNodeDeg = 49.6f;
    c.argumentOfPerihelionDeg = 286.5f; c.meanAnomalyAtEpochDeg = 19.4f;
    return c;
}

BodyConfig BodyConfig::jupiter() {
    BodyConfig c;
    c.bodyId = QStringLiteral("jupiter"); c.displayName = QStringLiteral("Jupiter");
    c.albedoTexture = QStringLiteral("jupiter.jpg");
    c.radiusKm = 69911.0f; c.rotationPeriodHours = 9.93f; c.axialTiltDegrees = 3.13f;
    c.hasAtmosphere = true;
    c.semiMajorAxisAU = 5.203f; c.eccentricity = 0.0489f;
    c.inclinationDeg = 1.3f; c.longitudeAscendingNodeDeg = 100.5f;
    c.argumentOfPerihelionDeg = 273.9f; c.meanAnomalyAtEpochDeg = 20.0f;
    return c;
}

BodyConfig BodyConfig::saturn() {
    BodyConfig c;
    c.bodyId = QStringLiteral("saturn"); c.displayName = QStringLiteral("Saturn");
    c.albedoTexture = QStringLiteral("saturn.jpg");
    c.ringTexture = QStringLiteral("saturn_ring.png");
    c.radiusKm = 58232.0f; c.rotationPeriodHours = 10.66f; c.axialTiltDegrees = 26.73f;
    c.hasRings = true; c.hasAtmosphere = true;
    c.semiMajorAxisAU = 9.537f; c.eccentricity = 0.0565f;
    c.inclinationDeg = 2.49f; c.longitudeAscendingNodeDeg = 113.7f;
    c.argumentOfPerihelionDeg = 339.1f; c.meanAnomalyAtEpochDeg = 317.0f;
    return c;
}

BodyConfig BodyConfig::uranus() {
    BodyConfig c;
    c.bodyId = QStringLiteral("uranus"); c.displayName = QStringLiteral("Uranus");
    c.albedoTexture = QStringLiteral("uranus.jpg");
    c.radiusKm = 25362.0f; c.rotationPeriodHours = -17.24f; c.axialTiltDegrees = 97.77f;  // retrograde
    c.hasAtmosphere = true;
    c.semiMajorAxisAU = 19.19f; c.eccentricity = 0.0457f;
    c.inclinationDeg = 0.77f; c.longitudeAscendingNodeDeg = 74.0f;
    c.argumentOfPerihelionDeg = 96.8f; c.meanAnomalyAtEpochDeg = 142.2f;
    return c;
}

BodyConfig BodyConfig::neptune() {
    BodyConfig c;
    c.bodyId = QStringLiteral("neptune"); c.displayName = QStringLiteral("Neptune");
    c.albedoTexture = QStringLiteral("neptune.jpg");
    c.radiusKm = 24622.0f; c.rotationPeriodHours = 16.11f; c.axialTiltDegrees = 28.32f;
    c.hasAtmosphere = true;
    c.semiMajorAxisAU = 30.07f; c.eccentricity = 0.0113f;
    c.inclinationDeg = 1.77f; c.longitudeAscendingNodeDeg = 131.8f;
    c.argumentOfPerihelionDeg = 276.3f; c.meanAnomalyAtEpochDeg = 256.2f;
    return c;
}
```

(Keep `BodyConfig::earth()` unchanged from Phase 1.)

- [ ] **Step 3: Extend `tests/test_bodyconfig.cpp`**

```cpp
void testAllFactoriesProduceValidId() {
    for (auto factory : {BodyConfig::sun, BodyConfig::moon, BodyConfig::mercury,
                         BodyConfig::venus, BodyConfig::earth, BodyConfig::mars,
                         BodyConfig::jupiter, BodyConfig::saturn,
                         BodyConfig::uranus, BodyConfig::neptune}) {
        BodyConfig c = factory();
        QVERIFY(!c.bodyId.isEmpty());
        QVERIFY(!c.displayName.isEmpty());
        QVERIFY(!c.albedoTexture.isEmpty());
        QVERIFY(c.radiusKm > 0.0f);
    }
}

void testSunIsEmissive() {
    QVERIFY(BodyConfig::sun().emissive);
}

void testSaturnHasRingsAndRingTexture() {
    BodyConfig c = BodyConfig::saturn();
    QVERIFY(c.hasRings);
    QVERIFY(!c.ringTexture.isEmpty());
}
```

Add `#include <QList>` if needed for the initializer-list iteration; otherwise call each factory individually.

- [ ] **Step 4: Build + run tests**

```bash
cmake --build F:\globe\build --target test_bodyconfig
cd F:\globe\build && ctest -R test_bodyconfig --output-on-failure
```

Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add -A
git commit -m "feat: BodyConfig factories for Sun, Moon, and all planets with orbital elements"
```

---

## Task 4: Emissive (Sun) and ring (Saturn) shaders

**Files:**
- Create: `src/celestial/shaders/star.vert`
- Create: `src/celestial/shaders/star.frag`
- Create: `src/celestial/shaders/ring.vert`
- Create: `src/celestial/shaders/ring.frag`

- [ ] **Step 1: Write `star.vert`**

Reuse `earth.vert` structure (same attributes/uniforms). Simplest: copy `earth.vert` to `star.vert`.

- [ ] **Step 2: Write `star.frag`**

Emissive: sample albedo at full intensity, no day/night, no normal/specular. Add a subtle limb falloff for corona feel:

```glsl
#version 330 core
in vec3 vLocalPos;
in vec2 vUv;
out vec4 FragColor;
uniform sampler2D uDay;
uniform float uHasDay;
uniform float uTime;
void main() {
    vec3 col = uHasDay > 0.5 ? texture(uDay, vUv).rgb : vec3(1.0, 0.85, 0.4);
    // gentle brightening toward center, slight pulse
    float pulse = 0.95 + 0.05 * sin(uTime * 0.5);
    FragColor = vec4(col * pulse, 1.0);
}
```

Match the `vLocalPos`/`vUv` varyings exactly as `earth.vert` outputs them (read `earth.vert` first and mirror names).

- [ ] **Step 3: Write `ring.vert` and `ring.frag`**

`ring.vert`: quad in the XZ plane (ring geometry built in code; see Task 5). Pass UV.

`ring.frag`:

```glsl
#version 330 core
in vec2 vUv;
out vec4 FragColor;
uniform sampler2D uRing;
void main() {
    FragColor = texture(uRing, vUv);
}
```

- [ ] **Step 4: Commit**

```bash
git add src/celestial/shaders
git commit -m "feat: emissive star shader and ring shader"
```

---

## Task 5: Body-type shader selection + ring pass in `CelestialBody`

**Files:**
- Modify: `src/celestial/CelestialBody.h`
- Modify: `src/celestial/CelestialBody.cpp`

- [ ] **Step 1: Add programs and ring geometry to `CelestialBody.h`**

```cpp
private:
    // ...existing...
    std::unique_ptr<QOpenGLShaderProgram> m_starProg;   // emissive (Sun)
    std::unique_ptr<QOpenGLShaderProgram> m_ringProg;   // Saturn rings
    std::unique_ptr<QOpenGLTexture> m_texRing;
    GLuint m_ringVao = 0, m_ringVbo = 0, m_ringIbo = 0;
    int m_ringIndexCount = 0;

    void buildRingQuad();   // annulus in XZ plane
    void loadRingTexture();
```

- [ ] **Step 2: In `initialize()`, load star + ring programs conditionally**

```cpp
void CelestialBody::initialize(QOpenGLFunctions_3_3_Core *gl) {
    m_gl = gl;
    if (m_config.emissive) {
        m_starProg = makeProgram("star.vert", "star.frag");
    } else {
        m_earthProg = makeProgram("earth.vert", "earth.frag");
        m_cloudProg = makeProgram("clouds.vert", "clouds.frag");
        m_atmoProg  = makeProgram("atmosphere.vert", "atmosphere.frag");
    }
    if (m_config.hasRings) {
        m_ringProg = makeProgram("ring.vert", "ring.frag");
        buildRingQuad();
    }
    buildSphere(128, 256);
    if (m_assets) loadTextures();
}
```

- [ ] **Step 3: In `loadTextures()`, use `BodyConfig` filenames via the new filename-based `AssetManager::image`**

```cpp
void CelestialBody::loadTextures() {
    const int cap = qMin(m_tierMaxSize, 4096);
    auto make = [](const QImage &img) {
        auto t = std::make_unique<QOpenGLTexture>(img.flipped(Qt::Vertical));
        t->setMinificationFilter(QOpenGLTexture::Linear);
        t->setMagnificationFilter(QOpenGLTexture::Linear);
        t->setWrapMode(QOpenGLTexture::Repeat);
        return t;
    };
    m_texDay = make(m_assets->image(m_config.albedoTexture, cap));
    if (!m_config.nightTexture.isEmpty())
        m_texNight = make(m_assets->image(m_config.nightTexture, cap));
    if (!m_config.normalTexture.isEmpty())
        m_texNormal = make(m_assets->image(m_config.normalTexture, cap));
    if (!m_config.specularTexture.isEmpty())
        m_texSpecular = make(m_assets->image(m_config.specularTexture, cap));
    if (m_config.hasRings && !m_config.ringTexture.isEmpty())
        m_texRing = make(m_assets->image(m_config.ringTexture, cap));
}
```

- [ ] **Step 4: Branch `render()` by body type**

Emissive path (Sun): bind `m_starProg`, set `uMVP`/`uModel`/`uHasDay`/`uTime`, draw sphere. No sun-direction shading, no atmosphere, no day/night.

Planet path: existing `earth` pass.

After the planet/sun draw, if `m_config.hasRings`, draw the ring annulus with `m_ringProg` (rotate into Saturn's equatorial plane via axial tilt).

- [ ] **Step 5: Build celestial lib**

```bash
cmake --build F:\globe\build --target celestial 2>&1 | tail -10
```

Expected: compiles.

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "feat: CelestialBody renders emissive Sun and ringed Saturn"
```

---

## Task 6: Per-body config directories in `ConfigManager`

**Files:**
- Modify: `src/celestial/ConfigManager.h`
- Modify: `src/celestial/ConfigManager.cpp`
- Modify: `tests/test_configmanager.cpp`
- Modify: `src/apps/earth/main.cpp` (pass bodyId)

- [ ] **Step 1: Add bodyId-aware constructor**

```cpp
// Header
explicit ConfigManager(const QString& bodyId = QStringLiteral("earth"),
                       QObject *parent = nullptr);
```

Implementation: base dir = `AppConfigLocation + "/" + bodyId` (or for earth, the legacy dir to preserve settings). Create the dir if missing.

```cpp
ConfigManager::ConfigManager(const QString& bodyId, QObject *parent) : QObject(parent) {
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    // Phase 1 compatibility: earth keeps the legacy top-level config.json
    m_path = (bodyId == QStringLiteral("earth")) ? (base + "/config.json")
                                                 : (base + "/" + bodyId + "/config.json");
    load();
}
```

- [ ] **Step 2: Add a test that each body gets its own path**

```cpp
void testPerBodyPath() {
    ConfigManager earth(QStringLiteral("earth"));
    ConfigManager mars(QStringLiteral("mars"));
    QVERIFY(earth.path().contains("config.json"));
    QVERIFY(mars.path().contains("mars"));
    QVERIFY(earth.path() != mars.path());
}
```

(Expose `path()` for testing, or compare via a side-effect.)

- [ ] **Step 3: Update `earth/main.cpp`**

```cpp
ConfigManager config(QStringLiteral("earth"));  // explicit; preserves legacy path
```

- [ ] **Step 4: Build + test**

```bash
cmake --build F:\globe\build && cd F:\globe\build && ctest --output-on-failure
```

Expected: all tests pass.

- [ ] **Step 5: Commit**

```bash
git add -A
git commit -m "feat: per-body config directories in ConfigManager"
```

---

## Task 7: Body executables (sun, moon, mercury … neptune) + CMake loop

**Files:**
- Create: `src/apps/{sun,moon,mercury,venus,mars,jupiter,saturn,uranus,neptune}/main.cpp`
- Modify: `CMakeLists.txt`
- Create: `src/celestial/BodyApp.h` (shared body-launch helper) — optional but cuts duplication

- [ ] **Step 1: Add a shared `runBodyApp()` helper to reduce per-main duplication**

Create `src/celestial/BodyApp.h/.cpp` exposing:

```cpp
int runBodyApp(int argc, char *argv[], const BodyConfig& config);
```

This encapsulates: QApplication, singleton mutex (key from `config.bodyId`), translator, AssetManager, SunModel (for planets — directionless for Sun), CameraController, CelestialWidget, TimeController, SystemTray, SettingsDialog, FullscreenWatcher. Body-app `main.cpp` then becomes 3 lines.

- [ ] **Step 2: Write `src/apps/sun/main.cpp`**

```cpp
#include "celestial/BodyApp.h"
#include "celestial/BodyConfig.h"
int main(int argc, char *argv[]) {
    return runBodyApp(argc, argv, BodyConfig::sun());
}
```

- [ ] **Step 3: Repeat for each body** (`moon`, `mercury`, … `neptune`), swapping the factory.

- [ ] **Step 4: Update `CMakeLists.txt` — loop over bodies**

```cmake
set(BODIES earth sun moon mercury venus mars jupiter saturn uranus neptune)
foreach(body ${BODIES})
    qt_add_executable(${body}
        WIN32 MACOSX_BUNDLE
        src/apps/${body}/main.cpp
    )
    target_link_libraries(${body} PRIVATE celestial)
endforeach()
```

Refactor the existing `earth` target into the loop. Keep translations + install + deploy for each (or at least `earth`).

- [ ] **Step 5: Build all**

```bash
cmake -S F:\globe -B F:\globe\build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build F:\globe\build 2>&1 | tail -20
```

Expected: all 10 executables build.

- [ ] **Step 6: Smoke-launch two bodies**

```bash
F:\globe\build\sun.exe   # then close
F:\globe\build\mars.exe  # then close
```

Expected: each shows its body spinning; sun is uniformly bright (emissive), mars shows day/night.

- [ ] **Step 7: Commit**

```bash
git add -A
git commit -m "feat: standalone widgets for Sun, Moon, and all planets"
```

---

## Task 8: Phase 2 verification

- [ ] **Step 1: Run all tests**

```bash
cd F:\globe\build && ctest --output-on-failure
```

Expected: all pass (including `test_assetmanager`, extended `test_bodyconfig`).

- [ ] **Step 2: windeployqt + launch each body, visually confirm**

```bash
foreach body in sun moon mercury venus mars jupiter saturn uranus neptune:
    windeployqt <body>.exe
    launch, verify it renders, close
```

Saturn must show rings; Sun must be uniformly bright (no terminator).

- [ ] **Step 3: Commit + tag**

```bash
git tag phase2-body-widgets
```

---

## Spec Coverage

| Spec item | Task |
|-----------|------|
| AssetManager multi-body | Task 1 |
| Body textures | Task 2 |
| BodyConfig factories + orbits | Task 3 |
| Emissive/ring shaders | Tasks 4–5 |
| Per-body config | Task 6 |
| Standalone executables + singleton per body | Task 7 |
| Tests | Tasks 1, 3, 6 |
