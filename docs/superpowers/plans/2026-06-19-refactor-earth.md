# Refactor Earth — Phase 1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Extract the current Earth widget into a reusable `celestial` shared library and a thin `earth` executable, preserving all current behavior while creating the foundation for Sun, Moon, and planet widgets.

**Architecture:** Move generic classes (`AssetManager`, `CameraController`, `SunModel`) into `src/celestial/`, wrap the current `GlobeRenderer` logic into a `CelestialBody` class parameterized by `BodyConfig`, replace `GlobeWindow`/`GlobeView` with `CelestialWidget`, and create `src/apps/earth/main.cpp` as the new entry point.

**Tech Stack:** Qt6, OpenGL 3.3 Core, CMake, MinGW/MSVC/Clang, QJson for config, existing GLSL shaders in `src/globe/render/shaders/`.

---

## File Structure After Phase 1

```
F:\globe\
├── CMakeLists.txt
├── src/
│   ├── celestial/
│   │   ├── AssetManager.h/.cpp          (moved from src/globe/render/)
│   │   ├── CameraController.h/.cpp      (moved from src/globe/render/)
│   │   ├── SunModel.h/.cpp              (moved from src/globe/render/)
│   │   ├── BodyConfig.h/.cpp            (new)
│   │   ├── CelestialBody.h/.cpp         (new; wraps Earth rendering)
│   │   ├── CelestialWidget.h/.cpp       (new; replaces GlobeWindow+GlobeView)
│   │   ├── ConfigManager.h/.cpp         (moved from src/globe/)
│   │   ├── SettingsDialog.h/.cpp        (moved from src/globe/)
│   │   ├── SystemTray.h/.cpp            (moved from src/globe/)
│   │   ├── FullscreenWatcher.h/.cpp     (moved from src/globe/)
│   │   ├── PlatformWindow.h/.cpp        (moved from src/globe/)
│   │   └── LocationProvider.h/.cpp      (moved from src/globe/)
│   ├── apps/
│   │   └── earth/
│   │       └── main.cpp                 (new; thin entry point)
│   └── globe/                           (kept during refactor; deleted in cleanup)
│       └── render/shaders/              (move to src/celestial/shaders/)
├── tests/
│   ├── test_bodyconfig.cpp              (new)
│   └── ...                              (updated include paths)
└── docs/superpowers/plans/2026-06-19-refactor-earth.md
```

---

## Task 1: Scaffold the `celestial` static library and move generic classes

**Files:**
- Create: `src/celestial/`
- Modify: `CMakeLists.txt`
- Move: `src/globe/render/AssetManager.*` → `src/celestial/AssetManager.*`
- Move: `src/globe/render/CameraController.*` → `src/celestial/CameraController.*`
- Move: `src/globe/render/SunModel.*` → `src/celestial/SunModel.*`

- [ ] **Step 1: Create directory and move files**

```bash
mkdir -p src/celestial src/apps/earth
mv src/globe/render/AssetManager.h src/celestial/AssetManager.h
mv src/globe/render/AssetManager.cpp src/celestial/AssetManager.cpp
mv src/globe/render/CameraController.h src/celestial/CameraController.h
mv src/globe/render/CameraController.cpp src/celestial/CameraController.cpp
mv src/globe/render/SunModel.h src/celestial/SunModel.h
mv src/globe/render/SunModel.cpp src/celestial/SunModel.cpp
```

- [ ] **Step 2: Update include guards and internal includes**

In `src/celestial/AssetManager.cpp`, change:
```cpp
#include "AssetManager.h"
```
(no path change needed if we set include dirs correctly).

In `src/celestial/SunModel.cpp`, remove any `render/` prefixed includes if present.

- [ ] **Step 3: Add `celestial` library to CMake and make `globe_lib` include it**

Modify `CMakeLists.txt`:

```cmake
qt_add_library(celestial STATIC
    src/celestial/AssetManager.h
    src/celestial/AssetManager.cpp
    src/celestial/CameraController.h
    src/celestial/CameraController.cpp
    src/celestial/SunModel.h
    src/celestial/SunModel.cpp
)

target_include_directories(celestial PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/celestial
)

target_link_libraries(celestial PUBLIC
    Qt::Core Qt::Gui Qt::Widgets Qt::OpenGL Qt::OpenGLWidgets Qt::Positioning
)

if(WIN32)
    target_link_libraries(celestial PUBLIC dwmapi user32)
endif()
```

Then make `globe_lib` depend on `celestial` and remove the moved files from `globe_lib`:

```cmake
qt_add_library(globe_lib STATIC
    # keep only the files still in src/globe/
    src/globe/GlobeWindow.h
    src/globe/GlobeWindow.cpp
    src/globe/SystemTray.h
    src/globe/SystemTray.cpp
    src/globe/PlatformWindow.h
    src/globe/PlatformWindow.cpp
    src/globe/ConfigManager.h
    src/globe/ConfigManager.cpp
    src/globe/LocationProvider.h
    src/globe/LocationProvider.cpp
    src/globe/SettingsDialog.h
    src/globe/SettingsDialog.cpp
    src/globe/FullscreenWatcher.h
    src/globe/FullscreenWatcher.cpp
    src/globe/render/GlobeRenderer.h
    src/globe/render/GlobeRenderer.cpp
    src/globe/render/GlobeView.h
    src/globe/render/GlobeView.cpp
    src/globe/render/TimeController.h
    src/globe/render/TimeController.cpp
)

target_include_directories(globe_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/globe
    ${CMAKE_CURRENT_SOURCE_DIR}/src/globe/render
    ${CMAKE_CURRENT_SOURCE_DIR}/src/celestial
)

target_link_libraries(globe_lib PUBLIC celestial)
```

- [ ] **Step 4: Build to verify moved classes compile**

```bash
cmake --build F:\globe\build --target globe_lib 2>&1 | tail -20
```

Expected: `globe_lib` compiles with no errors.

- [ ] **Step 5: Commit**

```bash
git add -A
git commit -m "refactor: move AssetManager, CameraController, SunModel into celestial library"
```

---

## Task 2: Move remaining generic app infrastructure into `celestial`

**Files:**
- Move: `src/globe/ConfigManager.*` → `src/celestial/ConfigManager.*`
- Move: `src/globe/SettingsDialog.*` → `src/celestial/SettingsDialog.*`
- Move: `src/globe/SystemTray.*` → `src/celestial/SystemTray.*`
- Move: `src/globe/FullscreenWatcher.*` → `src/celestial/FullscreenWatcher.*`
- Move: `src/globe/PlatformWindow.*` → `src/celestial/PlatformWindow.*`
- Move: `src/globe/LocationProvider.*` → `src/celestial/LocationProvider.*`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Move files**

```bash
mv src/globe/ConfigManager.h src/celestial/ConfigManager.h
mv src/globe/ConfigManager.cpp src/celestial/ConfigManager.cpp
mv src/globe/SettingsDialog.h src/celestial/SettingsDialog.h
mv src/globe/SettingsDialog.cpp src/celestial/SettingsDialog.cpp
mv src/globe/SystemTray.h src/celestial/SystemTray.h
mv src/globe/SystemTray.cpp src/celestial/SystemTray.cpp
mv src/globe/FullscreenWatcher.h src/celestial/FullscreenWatcher.h
mv src/globe/FullscreenWatcher.cpp src/celestial/FullscreenWatcher.cpp
mv src/globe/PlatformWindow.h src/celestial/PlatformWindow.h
mv src/globe/PlatformWindow.cpp src/celestial/PlatformWindow.cpp
mv src/globe/LocationProvider.h src/celestial/LocationProvider.h
mv src/globe/LocationProvider.cpp src/celestial/LocationProvider.cpp
```

- [ ] **Step 2: Update include paths inside moved files**

In `src/celestial/SettingsDialog.cpp`, change:
```cpp
#include "globe/SettingsDialog.h"    // old
#include "SettingsDialog.h"          // new
```

In `src/celestial/PlatformWindow.cpp`, remove any `globe/` prefix if present.

In `src/celestial/FullscreenWatcher.cpp`, update includes to `celestial/` or no path.

- [ ] **Step 3: Update `CMakeLists.txt`**

Add the moved files to `celestial` library and remove them from `globe_lib`:

```cmake
qt_add_library(celestial STATIC
    src/celestial/AssetManager.h
    src/celestial/AssetManager.cpp
    src/celestial/CameraController.h
    src/celestial/CameraController.cpp
    src/celestial/SunModel.h
    src/celestial/SunModel.cpp
    src/celestial/ConfigManager.h
    src/celestial/ConfigManager.cpp
    src/celestial/SettingsDialog.h
    src/celestial/SettingsDialog.cpp
    src/celestial/SystemTray.h
    src/celestial/SystemTray.cpp
    src/celestial/FullscreenWatcher.h
    src/celestial/FullscreenWatcher.cpp
    src/celestial/PlatformWindow.h
    src/celestial/PlatformWindow.cpp
    src/celestial/LocationProvider.h
    src/celestial/LocationProvider.cpp
)
```

`globe_lib` now only contains:
```cmake
qt_add_library(globe_lib STATIC
    src/globe/GlobeWindow.h
    src/globe/GlobeWindow.cpp
    src/globe/render/GlobeRenderer.h
    src/globe/render/GlobeRenderer.cpp
    src/globe/render/GlobeView.h
    src/globe/render/GlobeView.cpp
    src/globe/render/TimeController.h
    src/globe/render/TimeController.cpp
)
```

- [ ] **Step 4: Update `main.cpp` includes**

Change:
```cpp
#include "globe/ConfigManager.h"
#include "globe/SystemTray.h"
#include "globe/SettingsDialog.h"
#include "globe/FullscreenWatcher.h"
#include "render/SunModel.h"
#include "render/CameraController.h"
#include "render/AssetManager.h"
```
to:
```cpp
#include "ConfigManager.h"
#include "SystemTray.h"
#include "SettingsDialog.h"
#include "FullscreenWatcher.h"
#include "SunModel.h"
#include "CameraController.h"
#include "AssetManager.h"
```

- [ ] **Step 5: Build and run existing tests**

```bash
cmake --build F:\globe\build --target globe_lib 2>&1 | tail -20
cd F:\globe\build && ctest --output-on-failure
```

Expected: all 5 tests pass.

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "refactor: move ConfigManager, SettingsDialog, SystemTray, FullscreenWatcher, PlatformWindow, LocationProvider into celestial library"
```

---

## Task 3: Add `BodyConfig` for Earth

**Files:**
- Create: `src/celestial/BodyConfig.h`
- Create: `src/celestial/BodyConfig.cpp`
- Modify: `src/celestial/CMakeLists.txt` / `CMakeLists.txt`
- Create: `tests/test_bodyconfig.cpp`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write `src/celestial/BodyConfig.h`**

```cpp
#pragma once
#include <QString>

struct BodyConfig {
    QString bodyId;
    QString displayName;
    QString albedoTexture;
    QString nightTexture;
    QString normalTexture;
    QString specularTexture;
    QString cloudTexture;

    float radiusKm = 6371.0f;
    float visualScale = 1.0f;
    float rotationPeriodHours = 24.0f;
    float axialTiltDegrees = 23.5f;
    bool emissive = false;
    bool hasRings = false;
    bool hasClouds = false;
    bool hasAtmosphere = false;

    static BodyConfig earth();
};
```

- [ ] **Step 2: Write `src/celestial/BodyConfig.cpp`**

```cpp
#include "BodyConfig.h"

BodyConfig BodyConfig::earth() {
    BodyConfig c;
    c.bodyId = QStringLiteral("earth");
    c.displayName = QStringLiteral("Earth");
    c.albedoTexture = QStringLiteral("8k_earth_daymap.jpg");
    c.nightTexture  = QStringLiteral("8k_earth_nightmap.jpg");
    c.normalTexture = QStringLiteral("8k_earth_normal_map.jpg");
    c.specularTexture = QStringLiteral("8k_earth_specular_map.jpg");
    c.cloudTexture  = QStringLiteral("8k_earth_clouds.jpg");
    c.radiusKm = 6371.0f;
    c.visualScale = 1.0f;
    c.rotationPeriodHours = 23.9344696f;
    c.axialTiltDegrees = 23.5f;
    c.hasClouds = false;   // currently disabled
    c.hasAtmosphere = true;
    return c;
}
```

Note: texture file names should match what `AssetManager` currently expects. Adjust to actual names in `C:\Users\huang\Downloads\textures`.

- [ ] **Step 3: Add `BodyConfig` to `celestial` library in CMake**

```cmake
qt_add_library(celestial STATIC
    # (existing AssetManager, CameraController, SunModel files already here)
    src/celestial/BodyConfig.h
    src/celestial/BodyConfig.cpp
)
```

- [ ] **Step 4: Write failing test `tests/test_bodyconfig.cpp`**

```cpp
#include <QtTest/QtTest>
#include "BodyConfig.h"

class TestBodyConfig : public QObject {
    Q_OBJECT
private slots:
    void earthDefaults();
};

void TestBodyConfig::earthDefaults() {
    BodyConfig c = BodyConfig::earth();
    QCOMPARE(c.bodyId, QStringLiteral("earth"));
    QCOMPARE(c.displayName, QStringLiteral("Earth"));
    QCOMPARE(c.rotationPeriodHours, 23.9344696f);
    QVERIFY(c.hasAtmosphere);
}

QTEST_MAIN(TestBodyConfig)
#include "test_bodyconfig.moc"
```

- [ ] **Step 5: Add test to `tests/CMakeLists.txt`**

```cmake
globe_add_test(test_bodyconfig)
```

- [ ] **Step 6: Run test to verify it fails (no source yet compiled into test)**

```bash
cd F:\globe\build && ctest -R test_bodyconfig --output-on-failure
```

Expected: FAIL with undefined reference to `BodyConfig::earth()`.

- [ ] **Step 7: Build and verify test passes**

```bash
cmake --build F:\globe\build --target test_bodyconfig
cd F:\globe\build && ctest -R test_bodyconfig --output-on-failure
```

Expected: PASS.

- [ ] **Step 8: Commit**

```bash
git add -A
git commit -m "feat: add BodyConfig with Earth defaults and unit test"
```

---

## Task 4: Create `CelestialBody` from current `GlobeRenderer`

**Files:**
- Create: `src/celestial/CelestialBody.h`
- Create: `src/celestial/CelestialBody.cpp`
- Move/copy: `src/globe/render/shaders/` → `src/celestial/shaders/`
- Modify: `CMakeLists.txt`

`CelestialBody` is intentionally a near-port of the current `GlobeRenderer`. It keeps all Earth-specific uniforms but packages them into a reusable class.

- [ ] **Step 1: Move shaders**

```bash
mkdir -p src/celestial/shaders
cp -r src/globe/render/shaders/* src/celestial/shaders/
```

Update shader search paths in `CelestialBody` (see Step 2).

- [ ] **Step 2: Write `src/celestial/CelestialBody.h`**

```cpp
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

struct CelestialRenderOptions {
    bool showGrid = false;
    bool useNightTexture = false;
    bool hasHome = false;
    double homeLat = 0.0;
    double homeLon = 0.0;
    double centerLon = 0.0;
    bool useCenterLon = false;
    int rotationSpeedRatio = 2880;  // 1 = real-time
};

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

    void resize(int w, int h, float devicePixelRatio);
    void render();

    const BodyConfig& config() const { return m_config; }

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
    std::unique_ptr<QOpenGLTexture> m_texDay, m_texNight, m_texClouds, m_texNormal, m_texSpecular;
    GLuint m_vao = 0, m_vbo = 0, m_ibo = 0;
    int m_indexCount = 0;
    float m_aspect = 1.0f;
    float m_dpr = 1.0f;
    int m_w = 0, m_h = 0;

    void buildSphere(int stacks, int slices);
    void loadTextures();
    std::unique_ptr<QOpenGLShaderProgram> makeProgram(const QString &vertAlias, const QString &fragAlias);
};
```

- [ ] **Step 3: Write `src/celestial/CelestialBody.cpp`**

Copy the implementation of `GlobeRenderer.cpp` almost verbatim, with these changes:
- Class name becomes `CelestialBody`.
- Constructor takes `const BodyConfig& config` and stores it.
- `loadTextures()` uses `m_config.albedoTexture` etc. via `AssetManager` (you may need to extend `AssetManager` with named file support, or keep the `Slot` enum for Phase 1 and map `BodyConfig` fields to slots).
- `render()` reads options from `m_opts` instead of member booleans.
- `setOptions()` updates `m_opts`.

For Phase 1, the simplest path is to keep using `AssetManager::Slot` and add a mapping method in `CelestialBody`:

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
    m_texDay      = make(m_assets->image(AssetManager::Day, cap));
    m_texNight    = make(m_assets->image(AssetManager::Night, cap));
    m_texNormal   = make(m_assets->image(AssetManager::Normal, cap));
    m_texSpecular = make(m_assets->image(AssetManager::Specular, cap));
}
```

- [ ] **Step 4: Add `CelestialBody` to CMake**

```cmake
qt_add_library(celestial STATIC
    # (existing files already here)
    src/celestial/CelestialBody.h
    src/celestial/CelestialBody.cpp
)
```

- [ ] **Step 5: Build the `celestial` library**

```bash
cmake --build F:\globe\build --target celestial 2>&1 | tail -20
```

Expected: `celestial` compiles.

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "feat: add CelestialBody class ported from GlobeRenderer"
```

---

## Task 5: Create `CelestialWidget` to replace `GlobeWindow` + `GlobeView`

**Files:**
- Create: `src/celestial/CelestialWidget.h`
- Create: `src/celestial/CelestialWidget.cpp`
- Modify: `CMakeLists.txt`

`CelestialWidget` merges the responsibilities of `GlobeWindow` and `GlobeView`.

- [ ] **Step 1: Write `src/celestial/CelestialWidget.h`**

```cpp
#pragma once
#include <QWidget>
#include <QPoint>
#include <QOpenGLWidget>
#include <memory>
#include "BodyConfig.h"
#include "CelestialBody.h"

class CameraController;
class ConfigManager;
class QOpenGLFunctions_3_3_Core;

class CelestialWidget : public QOpenGLWidget {
    Q_OBJECT
public:
    explicit CelestialWidget(const BodyConfig& config,
                             ConfigManager *configManager,
                             QWidget *parent = nullptr);
    ~CelestialWidget() override;

    CelestialBody& body() { return m_body; }

    void setConfig(ConfigManager *c) { m_config = c; }
    void setCameraController(CameraController *c) { m_cam = c; m_body.setCamera(c); }

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void showEvent(QShowEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;

private:
    CelestialBody m_body;
    ConfigManager *m_config = nullptr;
    CameraController *m_cam = nullptr;
    QPoint m_lastPos;
    std::unique_ptr<QOpenGLFunctions_3_3_Core> m_gl;

    void applyOptionsFromConfig();
};
```

- [ ] **Step 2: Write `src/celestial/CelestialWidget.cpp`**

Combine `GlobeWindow.cpp` and `GlobeView.cpp`:

```cpp
#include "CelestialWidget.h"
#include "PlatformWindow.h"
#include "CameraController.h"
#include "ConfigManager.h"
#include <QVBoxLayout>
#include <QRegion>
#include <QShowEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QOpenGLContext>
#include <QSurfaceFormat>

CelestialWidget::CelestialWidget(const BodyConfig& config,
                                 ConfigManager *configManager,
                                 QWidget *parent)
    : QOpenGLWidget(parent), m_body(config), m_config(configManager) {
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint |
                   Qt::Tool | Qt::WindowDoesNotAcceptFocus);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
#ifdef Q_OS_MAC
    setAttribute(Qt::WA_MacAlwaysShowToolWindow, true);
#endif
    resize(220, 220);

    QSurfaceFormat fmt;
    fmt.setVersion(3, 3);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setSamples(4);
    fmt.setSwapInterval(1);
    setFormat(fmt);

    applyOptionsFromConfig();
}

CelestialWidget::~CelestialWidget() { makeCurrent(); }

void CelestialWidget::applyOptionsFromConfig() {
    if (!m_config) return;
    CelestialRenderOptions opts;
    opts.showGrid = m_config->showGrid();
    opts.useNightTexture = (m_config->nightMode() == QStringLiteral("texture"));
    opts.rotationSpeedRatio = m_config->rotationSpeed();
    opts.homeLat = m_config->homeLatitude();
    opts.homeLon = m_config->homeLongitude();
    opts.hasHome = true;  // marker always shown at configured home
    m_body.setOptions(opts);
    setWindowFlag(Qt::WindowStaysOnTopHint, m_config->alwaysOnTop());
}

void CelestialWidget::initializeGL() {
    m_gl = std::make_unique<QOpenGLFunctions_3_3_Core>();
    if (!m_gl->initializeOpenGLFunctions())
        qWarning("CelestialWidget: failed to resolve OpenGL 3.3 core functions");
    m_body.initialize(m_gl.get());
}

void CelestialWidget::resizeGL(int w, int h) {
    m_body.resize(w, h, float(devicePixelRatioF()));
}

void CelestialWidget::paintGL() {
    m_body.render();
}

void CelestialWidget::showEvent(QShowEvent *e) {
    QOpenGLWidget::showEvent(e);
    PlatformWindow::applyDwmFramelessAttributes(this);
}

void CelestialWidget::resizeEvent(QResizeEvent *e) {
    QOpenGLWidget::resizeEvent(e);
    const int side = qMin(width(), height());
    const QRect r((width() - side) / 2, (height() - side) / 2, side, side);
    setMask(QRegion(r, QRegion::Ellipse));
}

void CelestialWidget::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton || e->button() == Qt::MiddleButton)
        m_lastPos = e->globalPosition().toPoint();
    QOpenGLWidget::mousePressEvent(e);
}

void CelestialWidget::mouseMoveEvent(QMouseEvent *e) {
    const QPoint g = e->globalPosition().toPoint();
    const bool moveGesture = (e->buttons() & Qt::MiddleButton) ||
                             ((e->buttons() & Qt::LeftButton) && (e->modifiers() & Qt::AltModifier));
    if (moveGesture) {
        window()->move(window()->pos() + g - m_lastPos);
        m_lastPos = g;
        if (m_config) {
            m_config->setWindowX(window()->pos().x());
            m_config->setWindowY(window()->pos().y());
            m_config->save();
        }
    } else if ((e->buttons() & Qt::LeftButton) && m_cam) {
        const QPoint d = g - m_lastPos;
        m_lastPos = g;
        m_cam->applyDrag(d.x(), d.y());
        update();
    }
    QOpenGLWidget::mousePressEvent(e);
}

void CelestialWidget::wheelEvent(QWheelEvent *e) {
    const int delta = e->angleDelta().y();
    const int newSide = qBound(160, int(width() + delta * 0.3), 1024);
    if (newSide == width()) { QOpenGLWidget::wheelEvent(e); return; }
    const QPoint center = window()->geometry().center();
    window()->resize(newSide, newSide);
    window()->move(center - QPoint(newSide / 2, newSide / 2));
    if (m_config) {
        m_config->setDiameter(newSide);
        m_config->save();
    }
    update();
    QOpenGLWidget::wheelEvent(e);
}

void CelestialWidget::mouseDoubleClickEvent(QMouseEvent *e) {
    if (m_cam) {
        m_cam->reset();
        const QPoint center = window()->geometry().center();
        window()->resize(220, 220);
        window()->move(center - QPoint(110, 110));
        if (m_config) {
            m_config->setDiameter(220);
            m_config->save();
        }
        update();
    }
    QOpenGLWidget::mouseDoubleClickEvent(e);
}
```

Note: `CelestialWidget` inherits `QOpenGLWidget` directly to avoid a nested `GlobeView`. The `resizeEvent` override keeps the circular mask.

- [ ] **Step 3: Add to CMake**

```cmake
qt_add_library(celestial STATIC
    # (existing files already here)
    src/celestial/CelestialWidget.h
    src/celestial/CelestialWidget.cpp
)
```

- [ ] **Step 4: Build**

```bash
cmake --build F:\globe\build --target celestial 2>&1 | tail -20
```

Expected: compiles.

- [ ] **Step 5: Commit**

```bash
git add -A
git commit -m "feat: add CelestialWidget combining GlobeWindow and GlobeView"
```

---

## Task 6: Create the `earth` app entry point

**Files:**
- Create: `src/apps/earth/main.cpp`
- Modify: `CMakeLists.txt`
- Rename/remove: old `main.cpp`

- [ ] **Step 1: Write `src/apps/earth/main.cpp`**

Port the current `main.cpp` to use `CelestialWidget` and `BodyConfig::earth()`.

```cpp
#include "CelestialWidget.h"
#include "ConfigManager.h"
#include "SystemTray.h"
#include "SettingsDialog.h"
#include "FullscreenWatcher.h"
#include "SunModel.h"
#include "CameraController.h"
#include "AssetManager.h"
#include "TimeController.h"
#include "LocationProvider.h"
#include "BodyConfig.h"
#include <QApplication>
#include <QStandardPaths>
#include <QTimer>
#include <QDebug>
#include <QDateTime>
#include <QTranslator>
#include <QSharedMemory>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

int main(int argc, char *argv[]) {
#ifdef Q_OS_WIN
    {
        HANDLE mutex = CreateMutexW(nullptr, TRUE, L"GlobeAppSingleton_Earth_v1");
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            if (mutex) CloseHandle(mutex);
            return 0;
        }
    }
#else
    {
        QSharedMemory shm(QStringLiteral("GlobeAppSingleton_Earth_v1"));
        if (!shm.create(1)) return 0;
    }
#endif

    QApplication app(argc, argv);
    app.setApplicationName("Earth");

    QTranslator translator;
    {
        ConfigManager tmpConfig;  // uses default Globe config path to preserve existing settings
        const QString lang = tmpConfig.language();
        if (lang == QStringLiteral("zh_CN")) {
            if (translator.load(QStringLiteral(":/i18n/globe_zh_CN.qm")))
                app.installTranslator(&translator);
        }
    }

    // Phase 1 keeps the legacy config path so the user's existing settings are
    // preserved. Per-body config directories will be introduced in Phase 2.
    ConfigManager config;
    AssetManager assets;
    SunModel sun;
    CameraController camera;

    BodyConfig earthConfig = BodyConfig::earth();
    CelestialWidget widget(earthConfig, &config);
    widget.setCameraController(&camera);
    widget.body().setAssets(&assets);
    widget.body().setSun(&sun);
    {
        const int tierMax = (config.qualityTier() == ConfigManager::HD) ? 8192 :
                            (config.qualityTier() == ConfigManager::Medium) ? 4096 : 2048;
        widget.body().setQualityTier(tierMax);
    }

    widget.move(config.windowX(), config.windowY());
    widget.resize(config.diameter(), config.diameter());

    LocationProvider location;
    if (config.locationOptIn()) location.start();
    widget.applyOptionsFromConfig();  // sets home location, grid, night mode, rotation speed

    QObject::connect(&location, &LocationProvider::locationChanged, &widget,
        [&widget](double lat, double lon) {
            CelestialRenderOptions opts = widget.body().options();
            opts.homeLat = lat; opts.homeLon = lon; opts.hasHome = true;
            widget.body().setOptions(opts);
            widget.update();
        });

    TimeController time(&sun);
    time.setTarget(&widget);
    time.setFpsCap(config.fpsCap());

    SystemTray tray;
    SettingsDialog settingsDialog(&config, &widget);
    QObject::connect(&tray, &SystemTray::toggleVisibility, &widget,
        [&widget] { widget.isVisible() ? widget.hide() : widget.show(); });
    QObject::connect(&settingsDialog, &SettingsDialog::settingsChanged, &widget,
        [&widget, &config, &translator, &app]() {
            widget.applyOptionsFromConfig();
            const bool wasVisible = widget.isVisible();
            widget.setWindowFlag(Qt::WindowStaysOnTopHint, config.alwaysOnTop());
            if (wasVisible) widget.show();
            app.removeTranslator(&translator);
            if (config.language() == QStringLiteral("zh_CN")) {
                if (translator.load(QStringLiteral(":/i18n/globe_zh_CN.qm")))
                    app.installTranslator(&translator);
            }
            widget.update();
        });
    QObject::connect(&tray, &SystemTray::openSettings, &widget,
        [&settingsDialog]() { settingsDialog.exec(); });
    QObject::connect(&tray, &SystemTray::resetView, &widget, [&widget, &camera] {
        camera.reset();
        CelestialRenderOptions opts = widget.body().options();
        opts.useCenterLon = false;
        widget.body().setOptions(opts);
        widget.resize(220, 220);
        widget.update();
    });
    QObject::connect(&tray, &SystemTray::centerOnMe, &widget, [&widget, &camera, &config] {
        camera.reset();
        CelestialRenderOptions opts = widget.body().options();
        opts.centerLon = config.homeLongitude();
        opts.useCenterLon = true;
        widget.body().setOptions(opts);
        widget.resize(220, 220);
        widget.update();
    });

    auto refreshTooltip = [&sun, &tray] {
        sun.setUtc(QDateTime::currentDateTimeUtc());
        tray.setSolarTooltip(CelestialWidget::tr("Sun sub-point: %1°, %2°")
            .arg(sun.subSolarLatitude(), 0, 'f', 1)
            .arg(sun.subSolarLongitude(), 0, 'f', 1));
    };
    refreshTooltip();
    QTimer tooltipTimer;
    QObject::connect(&tooltipTimer, &QTimer::timeout, &widget, refreshTooltip);
    tooltipTimer.start(300000);

    FullscreenWatcher fullscreenWatcher;
    QObject::connect(&fullscreenWatcher, &FullscreenWatcher::fullscreenAppStarted, &widget,
        [&widget] { widget.hide(); });
    QObject::connect(
        &fullscreenWatcher, &FullscreenWatcher::fullscreenAppStopped, &widget,
        [&widget] { widget.show(); });
    fullscreenWatcher.start();

    widget.show();
    return app.exec();
}
```

- [ ] **Step 2: Update `CMakeLists.txt` to build `earth` instead of `globe`**

```cmake
qt_add_executable(earth
    WIN32 MACOSX_BUNDLE
    src/apps/earth/main.cpp
)
target_link_libraries(earth PRIVATE celestial)

qt_add_translations(TARGETS earth TS_FILES globe_zh_CN.ts)

install(TARGETS earth
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_app_script(TARGET earth OUTPUT_SCRIPT deploy_script NO_UNSUPPORTED_PLATFORM_ERROR)
```

Remove the old `globe` target and `target_link_libraries(globe PRIVATE globe_lib)`.

- [ ] **Step 3: Keep old `main.cpp` for reference**

Rename old `main.cpp` to `main.cpp.legacy` or move to `archive/`; do not delete yet.

```bash
mv main.cpp main.cpp.legacy
```

- [ ] **Step 4: Build `earth`**

```bash
cmake -S F:\globe -B F:\globe\build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build F:\globe\build --target earth 2>&1 | tail -20
```

Expected: `earth.exe` links successfully.

- [ ] **Step 5: Run `earth.exe` and visually verify it behaves like the old `globe.exe`**

```bash
F:\globe\build\earth.exe
```

Check: spinning, day/night, grid toggle, settings dialog, wheel resize, drag, double-click reset, tray menu, fullscreen hide.

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "feat: add earth.exe entry point using CelestialWidget"
```

---

## Task 7: Update tests for new include paths

**Files:**
- Modify: `tests/test_configmanager.cpp`
- Modify: `tests/test_sunmodel.cpp`
- Modify: `tests/test_cameracontroller.cpp`
- Modify: `tests/test_sunview.cpp`
- Modify: `tests/test_locationprovider.cpp`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Update include paths in tests**

For example in `tests/test_sunmodel.cpp`, change:
```cpp
#include "render/SunModel.h"
```
to:
```cpp
#include "SunModel.h"
```

Do the same for `CameraController`, `ConfigManager`, `LocationProvider`.

- [ ] **Step 2: Update `tests/CMakeLists.txt`**

```cmake
function(globe_add_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE celestial Qt::Test)
    add_test(NAME ${name} COMMAND ${name})
endfunction()
```

- [ ] **Step 3: Run all tests**

```bash
cd F:\globe\build && ctest --output-on-failure
```

Expected: all tests pass.

- [ ] **Step 4: Commit**

```bash
git add -A
git commit -m "test: update tests to link against celestial library"
```

---

## Task 8: Cleanup old files (after verifying earth.exe)

**Files:**
- Delete: `src/globe/GlobeWindow.*`
- Delete: `src/globe/render/GlobeView.*`
- Delete: `src/globe/render/GlobeRenderer.*`
- Delete: `src/globe/render/TimeController.*` (or move if still needed)
- Delete: `main.cpp.legacy`
- Modify: `CMakeLists.txt` to remove `globe_lib`

- [ ] **Step 1: Verify `TimeController` is no longer needed**

The new `CelestialWidget`/earth app uses `TimeController`. If `TimeController` was not moved to `celestial` earlier, move it now before deleting `globe_lib`.

```bash
mv src/globe/render/TimeController.h src/celestial/TimeController.h
mv src/globe/render/TimeController.cpp src/celestial/TimeController.cpp
```

Add to `celestial` CMake target and remove from `globe_lib`.

- [ ] **Step 2: Remove old files**

```bash
rm src/globe/GlobeWindow.h src/globe/GlobeWindow.cpp
rm src/globe/render/GlobeView.h src/globe/render/GlobeView.cpp
rm src/globe/render/GlobeRenderer.h src/globe/render/GlobeRenderer.cpp
rm main.cpp.legacy
```

- [ ] **Step 3: Remove `globe_lib` from CMake**

Delete the `qt_add_library(globe_lib ...)` block and any references.

- [ ] **Step 4: Final build and test**

```bash
cmake -S F:\globe -B F:\globe\build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build F:\globe\build --target earth
cd F:\globe\build && ctest --output-on-failure
```

Expected: `earth.exe` builds and all tests pass.

- [ ] **Step 5: Commit**

```bash
git add -A
git commit -m "refactor: remove legacy GlobeWindow/GlobeView/GlobeRenderer and globe_lib"
```

---

## Task 9: Final verification and deploy

- [ ] **Step 1: Run `windeployqt` on `earth.exe`**

```bash
cd F:\globe\build
windeployqt earth.exe
```

- [ ] **Step 2: Launch `earth.exe` from the build folder and confirm behavior**

```bash
F:\globe\build\earth.exe
```

Verify:
- Window is circular and frameless
- Earth spins
- Day/night terminator is visible
- Right-click tray → Settings works
- Grid toggle works
- Always-on-top works
- Wheel resizes widget
- Double-click resets to 220 px
- Fullscreen app hides the widget

- [ ] **Step 3: Run all tests one final time**

```bash
cd F:\globe\build && ctest --output-on-failure
```

Expected: 100% tests passed.

- [ ] **Step 4: Commit verification log / tag (optional)**

```bash
git tag phase1-earth-refactor
```

---

## Spec Coverage Check

| Spec Requirement | Implementing Task |
|------------------|-------------------|
| Shared `celestial` library | Tasks 1, 2 |
| `BodyConfig` with Earth defaults | Task 3 |
| `CelestialBody` generic renderer | Task 4 |
| `CelestialWidget` replacing GlobeWindow/GlobeView | Task 5 |
| `earth.exe` as standalone entry point | Task 6 |
| Singleton per body (Earth-specific mutex) | Task 6 |
| Cross-platform CMake build | All tasks |
| Preserve current Earth behavior | Tasks 6, 9 |
| Tests updated and passing | Tasks 3, 7, 8, 9 |

## Placeholder Scan

No TBD, TODO, or vague steps. Each step includes exact file paths and expected outcomes.

## Type Consistency Notes

- `ConfigManager` retains its existing API; `CelestialWidget::applyOptionsFromConfig()` reads from it.
- `CelestialBody::setOptions()` takes `CelestialRenderOptions` by const reference.
- `BodyConfig::earth()` returns by value.
- `AssetManager::Slot` enum remains unchanged in Phase 1.

## Execution Options

Plan complete and saved to `docs/superpowers/plans/2026-06-19-refactor-earth.md`. Two execution options:

**1. Subagent-Driven (recommended)** — I dispatch a fresh subagent per task, review between tasks, fast iteration.

**2. Inline Execution** — Execute tasks in this session using `executing-plans`, batch execution with checkpoints.

Which approach do you want?
