# Globe Desktop Widget Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a native Qt6/C++ desktop widget that renders a photorealistic, real-time Earth globe (day/night terminator from the system clock, moving clouds, atmosphere glow) as a frameless, always-on-top, circular, resizable window.

**Architecture:** Layered OpenGL renderer inside a `QOpenGLWidget`, driven by a `SunModel` that computes the sub-solar point from UTC. A Seelie-style frameless shell (`QWidget` + DWM helper on Windows) hosts the view, with system tray, config persistence, and optional permission-gated location features.

**Tech Stack:** Qt 6.5+ (Widgets, Gui, OpenGL, OpenGLWidgets), C++17, CMake, Qt Test, raw GLSL, NASA Blue/Black Marble textures (bundled).

**Spec:** `docs/superpowers/specs/2026-06-18-globe-widget-design.md`

---

## File Structure

**Created:**
- `src/globe/ConfigManager.h/.cpp` — JSON config (position, diameter, quality, fps cap, cloud speed, home lon, location opt-in).
- `src/globe/PlatformWindow.h/.cpp` — cross-platform frameless/DWM helper (ported from Seelie).
- `src/globe/SystemTray.h/.cpp` — tray icon + context menu.
- `src/globe/GlobeWindow.h/.cpp` — frameless translucent always-on-top shell hosting `GlobeView`.
- `src/globe/LocationProvider.h/.cpp` — platform geolocation abstraction (Windows WinRT / macOS CoreLocation / Linux GeoClue).
- `src/globe/render/GlobeView.h/.cpp` — `QOpenGLWidget` owning the GL context & render loop.
- `src/globe/render/GlobeRenderer.h/.cpp` — sphere VAOs, shaders, three-layer draw.
- `src/globe/render/SunModel.h/.cpp` — sub-solar point / sun direction from UTC (pure, testable).
- `src/globe/render/CameraController.h/.cpp` — sun-centric base + drag/zoom offset + reset.
- `src/globe/render/AssetManager.h/.cpp` — loads HD textures, quality tiers, procedural fallbacks.
- `src/globe/render/TimeController.h/.cpp` — QTimer driving sun recomputation + fps cap.
- `src/globe/render/shaders.qrc` — bundled GLSL sources.
- `src/globe/render/shaders/{earth,clouds,atmosphere}.vert/.frag` — shaders.
- `assets/textures/README.md` — where to place `day.jpg`, `night.jpg`, `clouds.png`.
- `scripts/fetch_textures.ps1` — downloads NASA HD textures.
- `installer/globe.iss` — Inno Setup script.
- `tests/test_sunmodel.cpp`, `tests/test_configmanager.cpp`, `tests/test_cameracontroller.cpp`, `tests/test_locationprovider.cpp`.

**Modified:**
- `CMakeLists.txt` — Qt modules, globes sources, shaders resource, tests, deploy.
- `main.cpp` — construct `ConfigManager`, `LocationProvider`, `GlobeWindow`.

**Removed:**
- `mainwindow.h`, `mainwindow.cpp`, `mainwindow.ui` (replaced by `GlobeWindow`).

---

## Task 1: Restructure project skeleton

**Files:**
- Modify: `CMakeLists.txt`
- Modify: `main.cpp`
- Create: `src/globe/GlobeWindow.h`, `src/globe/GlobeWindow.cpp`
- Remove: `mainwindow.h`, `mainwindow.cpp`, `mainwindow.ui`

- [ ] **Step 1: Replace `CMakeLists.txt`**

```cmake
cmake_minimum_required(VERSION 3.19)
project(globe LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets LinguistTools Test)

qt_standard_project_setup()

qt_add_executable(globe
    WIN32 MACOSX_BUNDLE
    main.cpp
    src/globe/GlobeWindow.h
    src/globe/GlobeWindow.cpp
)

target_link_libraries(globe PRIVATE
    Qt::Core Qt::Gui Qt::Widgets Qt::OpenGL Qt::OpenGLWidgets
)

qt_add_translations(TARGETS globe TS_FILES globe_zh_CN.ts)

install(TARGETS globe
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_app_script(TARGET globe OUTPUT_SCRIPT deploy_script NO_UNSUPPORTED_PLATFORM_ERROR)
install(SCRIPT ${deploy_script})

# Tests
enable_testing()
add_subdirectory(tests)
```

- [ ] **Step 2: Create `tests/CMakeLists.txt`**

```cmake
find_package(Qt6 REQUIRED COMPONENTS Test)

function(globe_add_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE globe PRIVATE Qt::Test)
    add_test(NAME ${name} COMMAND ${name})
endfunction()
```
(The individual test targets are appended by later tasks.)

- [ ] **Step 3: Create minimal `GlobeWindow.h`**

```cpp
#pragma once
#include <QWidget>

class GlobeWindow : public QWidget {
    Q_OBJECT
public:
    explicit GlobeWindow(QWidget *parent = nullptr);
};
```

- [ ] **Step 4: Create minimal `GlobeWindow.cpp`**

```cpp
#include "GlobeWindow.h"
#include <QPainter>

GlobeWindow::GlobeWindow(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground);
    resize(360, 360);
}

void GlobeWindow::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(QColor(20, 40, 90));
    p.setPen(Qt::NoPen);
    p.drawEllipse(rect());
}
```
Add the `paintEvent` override declaration to `GlobeWindow.h` under `public:` → `protected: void paintEvent(QPaintEvent *) override;`.

- [ ] **Step 5: Replace `main.cpp`**

```cpp
#include "GlobeWindow.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    GlobeWindow w;
    w.show();
    return app.exec();
}
```

- [ ] **Step 6: Remove the old mainwindow files**

```bash
git rm mainwindow.h mainwindow.cpp mainwindow.ui
```

- [ ] **Step 7: Configure & build**

Run: `cmake -S . -B build -DCMAKE_PREFIX_PATH=<your Qt6 path>`
Then: `cmake --build build --config Release`
Expected: builds a `globe` executable that shows a translucent window with a blue circle.

- [ ] **Step 8: Commit**

```bash
git add -A
git commit -m "feat: restructure to GlobeWindow skeleton"
```

---

## Task 2: ConfigManager (TDD)

**Files:**
- Create: `src/globe/ConfigManager.h`, `src/globe/ConfigManager.cpp`
- Test: `tests/test_configmanager.cpp`
- Modify: `CMakeLists.txt`, `tests/CMakeLists.txt`

- [ ] **Step 1: Write the failing test `tests/test_configmanager.cpp`**

```cpp
#include <QtTest>
#include "ConfigManager.h"
#include <QTemporaryDir>

class TestConfigManager : public QObject { Q_OBJECT
private slots:
    void savesAndLoadsValues() {
        QTemporaryDir dir;
        ConfigManager c(dir.path());
        c.setWindowX(123); c.setWindowY(456);
        c.setDiameter(420); c.setQualityTier(ConfigManager::HD);
        c.setFpsCap(30); c.setLocationOptIn(true); c.setHomeLongitude(116.4);
        c.save();

        ConfigManager c2(dir.path());
        QCOMPARE(c2.windowX(), 123);
        QCOMPARE(c2.windowY(), 456);
        QCOMPARE(c2.diameter(), 420);
        QCOMPARE(c2.qualityTier(), ConfigManager::HD);
        QCOMPARE(c2.fpsCap(), 30);
        QVERIFY(c2.locationOptIn());
        QCOMPARE(c2.homeLongitude(), 116.4);
    }
    void clampsDiameter() {
        ConfigManager c; // temp default location
        c.setDiameter(10);
        QCOMPARE(c.diameter(), 160);     // min
        c.setDiameter(100000);
        QCOMPARE(c.diameter(), 1024);    // max
    }
};

QTEST_GUILESS_MAIN(TestConfigManager)
#include "test_configmanager.moc"
```

- [ ] **Step 2: Add test target in `tests/CMakeLists.txt`**

Append:
```cmake
target_include_directories(globe PRIVATE ${CMAKE_SOURCE_DIR}/src/globe)
globe_add_test(test_configmanager)
```
Also expose `src/globe` to the `globe` target in the root `CMakeLists.txt`:
```cmake
target_include_directories(globe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/globe)
```

- [ ] **Step 3: Run test to verify it fails**

Run: `cmake --build build --target test_configmanager && ctest --test-dir build -R test_configmanager --output-on-failure`
Expected: FAIL (ConfigManager does not exist).

- [ ] **Step 4: Create `src/globe/ConfigManager.h`**

```cpp
#pragma once
#include <QObject>

class ConfigManager : public QObject {
    Q_OBJECT
public:
    enum QualityTier { Low = 0, Medium = 1, HD = 2 };
    Q_ENUM(QualityTier)

    explicit ConfigManager(QString dir = QString(), QObject *parent = nullptr);

    int windowX() const;        void setWindowX(int v);
    int windowY() const;        void setWindowY(int v);
    int diameter() const;       void setDiameter(int v);   // clamped [160,1024]
    QualityTier qualityTier() const; void setQualityTier(QualityTier t);
    int fpsCap() const;         void setFpsCap(int v);     // 30/60
    double cloudSpeed() const;  void setCloudSpeed(double v);
    bool locationOptIn() const; void setLocationOptIn(bool v);
    double homeLongitude() const; void setHomeLongitude(double v);

    void load();
    void save();

private:
    QString m_path;
    int m_x = 100, m_y = 100, m_diameter = 360, m_fpsCap = 60;
    QualityTier m_tier = HD;
    double m_cloudSpeed = 1.0;
    bool m_locationOptIn = false;
    double m_homeLon = 0.0;
    static int clampDiameter(int v);
};
```

- [ ] **Step 5: Create `src/globe/ConfigManager.cpp`**

```cpp
#include "ConfigManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

int ConfigManager::clampDiameter(int v) { return v < 160 ? 160 : (v > 1024 ? 1024 : v); }

ConfigManager::ConfigManager(QString dir, QObject *parent) : QObject(parent) {
    if (dir.isEmpty())
        dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    m_path = dir + "/config.json";
    load();
}

int ConfigManager::windowX() const { return m_x; }
void ConfigManager::setWindowX(int v) { m_x = v; }
int ConfigManager::windowY() const { return m_y; }
void ConfigManager::setWindowY(int v) { m_y = v; }
int ConfigManager::diameter() const { return m_diameter; }
void ConfigManager::setDiameter(int v) { m_diameter = clampDiameter(v); }
ConfigManager::QualityTier ConfigManager::qualityTier() const { return m_tier; }
void ConfigManager::setQualityTier(QualityTier t) { m_tier = t; }
int ConfigManager::fpsCap() const { return m_fpsCap; }
void ConfigManager::setFpsCap(int v) { m_fpsCap = (v == 30 ? 30 : 60); }
double ConfigManager::cloudSpeed() const { return m_cloudSpeed; }
void ConfigManager::setCloudSpeed(double v) { m_cloudSpeed = v; }
bool ConfigManager::locationOptIn() const { return m_locationOptIn; }
void ConfigManager::setLocationOptIn(bool v) { m_locationOptIn = v; }
double ConfigManager::homeLongitude() const { return m_homeLon; }
void ConfigManager::setHomeLongitude(double v) { m_homeLon = v; }

void ConfigManager::load() {
    QFile f(m_path);
    if (!f.open(QIODevice::ReadOnly)) return;
    const auto obj = QJsonDocument::fromJson(f.readAll()).object();
    m_x = obj.value("windowX").toInt(m_x);
    m_y = obj.value("windowY").toInt(m_y);
    m_diameter = clampDiameter(obj.value("diameter").toInt(m_diameter));
    m_tier = static_cast<QualityTier>(obj.value("quality").toInt(static_cast<int>(m_tier)));
    m_fpsCap = (obj.value("fpsCap").toInt(m_fpsCap) == 30 ? 30 : 60);
    m_cloudSpeed = obj.value("cloudSpeed").toDouble(m_cloudSpeed);
    m_locationOptIn = obj.value("locationOptIn").toBool(m_locationOptIn);
    m_homeLon = obj.value("homeLongitude").toDouble(m_homeLon);
}

void ConfigManager::save() {
    QJsonObject o;
    o["windowX"] = m_x; o["windowY"] = m_y; o["diameter"] = m_diameter;
    o["quality"] = static_cast<int>(m_tier); o["fpsCap"] = m_fpsCap;
    o["cloudSpeed"] = m_cloudSpeed; o["locationOptIn"] = m_locationOptIn;
    o["homeLongitude"] = m_homeLon;
    QFile f(m_path);
    if (f.open(QIODevice::WriteOnly))
        f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
}
```

- [ ] **Step 6: Add `ConfigManager.cpp/.h` to the `globe` target** in root `CMakeLists.txt` `qt_add_executable(globe ...)`.

- [ ] **Step 7: Run test to verify it passes**

Run: `cmake --build build --target test_configmanager && ctest --test-dir build -R test_configmanager --output-on-failure`
Expected: PASS (2 tests).

- [ ] **Step 8: Commit**

```bash
git add -A
git commit -m "feat: ConfigManager with JSON persistence + tests"
```

---

## Task 3: SunModel (TDD)

**Files:**
- Create: `src/globe/render/SunModel.h`, `src/globe/render/SunModel.cpp`
- Test: `tests/test_sunmodel.cpp`

- [ ] **Step 1: Write the failing test `tests/test_sunmodel.cpp`**

```cpp
#include <QtTest>
#include <QDateTime>
#include <QVector3D>
#include "SunModel.h"

class TestSunModel : public QObject { Q_OBJECT
private slots:
    void directionIsUnitLength() {
        SunModel s;
        s.setUtc(QDateTime::currentDateTimeUtc());
        QVERIFY2(qAbs(s.sunDirection().length() - 1.0) < 1e-6, "not unit length");
    }
    void declinationBoundedToAxialTilt() {
        SunModel s;
        for (int month = 1; month <= 12; ++month) {
            s.setUtc(QDateTime(QDate(2024, month, 15), QTime(12, 0), Qt::UTC));
            const double dec = s.subSolarLatitude();
            QVERIFY2(dec >= -23.5 && dec <= 23.5, qPrintable(QString("dec=%1").arg(dec)));
        }
    }
    void longitudeBounded() {
        SunModel s;
        s.setUtc(QDateTime::currentDateTimeUtc());
        const double lon = s.subSolarLongitude();
        QVERIFY(lon >= -180.0 && lon <= 180.0);
    }
    void juneSolsticeDeclinationPositive() {
        SunModel s;
        s.setUtc(QDateTime(QDate(2024, 6, 21), QTime(3, 0), Qt::UTC)); // ~June solstice
        QVERIFY2(s.subSolarLatitude() > 22.0, qPrintable(QString("dec=%1").arg(s.subSolarLatitude())));
    }
};

QTEST_GUILESS_MAIN(TestSunModel)
#include "test_sunmodel.moc"
```

- [ ] **Step 2: Add `src/globe/render` to include dirs** (root CMake: `target_include_directories(globe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)`) and add the test target:
```cmake
globe_add_test(test_sunmodel)
```

- [ ] **Step 3: Run test to verify it fails**

Run: `cmake --build build --target test_sunmodel && ctest --test-dir build -R test_sunmodel --output-on-failure`
Expected: FAIL (SunModel missing).

- [ ] **Step 4: Create `src/globe/render/SunModel.h`**

```cpp
#pragma once
#include <QDateTime>
#include <QVector3D>

// Computes the sub-solar point and a unit sun direction in Earth-fixed (ECEF)
// coordinates, given a UTC time. Implements a standard low-precision solar
// position algorithm (good to ~1 degree, sufficient for a visual widget).
class SunModel {
public:
    void setUtc(const QDateTime &utc) { m_utc = utc.toUTC(); }
    const QDateTime &utc() const { return m_utc; }

    double subSolarLatitude() const;   // degrees, [-23.5, 23.5] roughly
    double subSolarLongitude() const;  // degrees, [-180, 180]
    QVector3D sunDirection() const;    // unit, ECEF

private:
    QDateTime m_utc = QDateTime::currentDateTimeUtc();

    static double toRadians(double deg);
    static double toDegrees(double rad);
    static double wrap180(double deg);
    double julianDay() const;
};
```

- [ ] **Step 5: Create `src/globe/render/SunModel.cpp`**

```cpp
#include "SunModel.h"
#include <QtMath>

double SunModel::toRadians(double d) { return d * M_PI / 180.0; }
double SunModel::toDegrees(double r) { return r * 180.0 / M_PI; }
double SunModel::wrap180(double d) {
    while (d > 180.0) d -= 360.0;
    while (d < -180.0) d += 360.0;
    return d;
}

double SunModel::julianDay() const {
    const qint64 ms = m_utc.toMSecsSinceEpoch();
    return 2440587.5 + (ms / 86400000.0);
}

double SunModel::subSolarLatitude() const {
    const double n = julianDay() - 2451545.0;
    const double L = wrap180(280.460 + 0.9856474 * n);   // mean longitude
    const double g = toRadians(wrap180(357.528 + 0.9856003 * n)); // mean anomaly
    const double lambda = toRadians(L + 1.915 * std::sin(g) + 0.020 * std::sin(2.0 * g));
    const double eps = toRadians(23.439 - 0.0000004 * n); // obliquity
    return toDegrees(std::asin(std::sin(eps) * std::sin(lambda))); // declination
}

double SunModel::subSolarLongitude() const {
    const double n = julianDay() - 2451545.0;
    const double L = wrap180(280.460 + 0.9856474 * n);
    const double g = toRadians(wrap180(357.528 + 0.9856003 * n));
    const double lambda = L + 1.915 * std::sin(g) + 0.020 * std::sin(2.0 * g); // ecliptic lon (deg)
    const double eps = toRadians(23.439 - 0.0000004 * n);
    // Right ascension (degrees)
    const double ra = toDegrees(std::atan2(std::cos(eps) * std::sin(toRadians(lambda)),
                                           std::cos(toRadians(lambda))));
    // Greenwich Mean Sidereal Time (hours)
    const double gmstHours = std::fmod(18.697374558 + 24.06570982441908 * n, 24.0);
    // Sub-solar longitude (east positive): RA - GMST*15
    return wrap180(ra - gmstHours * 15.0);
}

QVector3D SunModel::sunDirection() const {
    const double lat = toRadians(subSolarLatitude());
    const double lon = toRadians(subSolarLongitude());
    return QVector3D(std::cos(lat) * std::cos(lon),
                     std::cos(lat) * std::sin(lon),
                     std::sin(lat)).normalized();
}
```

- [ ] **Step 6: Run test to verify it passes**

Run: `cmake --build build --target test_sunmodel && ctest --test-dir build -R test_sunmodel --output-on-failure`
Expected: PASS (4 tests).

- [ ] **Step 7: Commit**

```bash
git add -A
git commit -m "feat: SunModel — sub-solar point & sun direction from UTC"
```

---

## Task 4: CameraController (TDD)

**Files:**
- Create: `src/globe/render/CameraController.h`, `src/globe/render/CameraController.cpp`
- Test: `tests/test_cameracontroller.cpp`

- [ ] **Step 1: Write the failing test `tests/test_cameracontroller.cpp`**

```cpp
#include <QtTest>
#include "CameraController.h"

class TestCameraController : public QObject { Q_OBJECT
private slots:
    void startsAtZeroOffset() {
        CameraController c;
        QCOMPARE(c.offsetYaw(), 0.0);
        QCOMPARE(c.offsetPitch(), 0.0);
    }
    void dragChangesOffset() {
        CameraController c;
        c.applyDrag(100, 40);
        QVERIFY(c.offsetYaw() != 0.0);
        QVERIFY(c.offsetPitch() != 0.0);
    }
    void resetClearsOffset() {
        CameraController c;
        c.applyDrag(100, 40);
        c.reset();
        QCOMPARE(c.offsetYaw(), 0.0);
        QCOMPARE(c.offsetPitch(), 0.0);
    }
    void zoomClamped() {
        CameraController c;
        c.applyZoom(1e9);
        QVERIFY(c.zoom() <= CameraController::kMaxZoom);
        c.applyZoom(-1e9);
        QVERIFY(c.zoom() >= CameraController::kMinZoom);
    }
};

QTEST_GUILESS_MAIN(TestCameraController)
#include "test_cameracontroller.moc"
```

- [ ] **Step 2: Add test target `globe_add_test(test_cameracontroller)`** in `tests/CMakeLists.txt`.

- [ ] **Step 3: Run test to verify it fails**

Run: `cmake --build build --target test_cameracontroller && ctest --test-dir build -R test_cameracontroller --output-on-failure`
Expected: FAIL.

- [ ] **Step 4: Create `src/globe/render/CameraController.h`**

```cpp
#pragma once
#include <QMatrix4x4>

// Sun-centric base orientation (set by the renderer from SunModel) plus a
// user yaw/pitch/zoom offset that drag & wheel manipulate. reset() re-locks
// to the real-time sun-centric view.
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
```

- [ ] **Step 5: Create `src/globe/render/CameraController.cpp`**

```cpp
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
    r.rotate(toDegrees(m_pitch), 1.0f, 0.0f, 0.0f);
    r.rotate(toDegrees(m_yaw),   0.0f, 1.0f, 0.0f);
    return r;
}
```

- [ ] **Step 6: Run test to verify it passes**

Run: `cmake --build build --target test_cameracontroller && ctest --test-dir build -R test_cameracontroller --output-on-failure`
Expected: PASS (4 tests).

- [ ] **Step 7: Commit**

```bash
git add -A
git commit -m "feat: CameraController — sun-centric base + drag/zoom offset"
```

---

## Task 5: GLSL shaders (Earth, clouds, atmosphere)

**Files:**
- Create: `src/globe/render/shaders/earth.vert`, `src/globe/render/shaders/earth.frag`
- Create: `src/globe/render/shaders/clouds.vert`, `src/globe/render/shaders/clouds.frag`
- Create: `src/globe/render/shaders/atmosphere.vert`, `src/globe/render/shaders/atmosphere.frag`
- Create: `src/globe/render/shaders.qrc`

- [ ] **Step 1: `earth.vert`** — passes through local vertex position (also used as the normal for a unit sphere), and the clip position.

```glsl
#version 330 core
layout(location = 0) in vec3 aPos;   // unit-sphere vertex == normal
out vec3 vLocal;
out vec3 vWorld;
uniform mat4 uMVP;
uniform mat4 uModel;
void main() {
    vLocal = aPos;
    vWorld = vec3(uModel * vec4(aPos, 1.0));
    gl_Position = uMVP * vec4(aPos, 1.0);
}
```

- [ ] **Step 2: `earth.frag`** — day/night blend from `sunDir`, equirectangular sampling, circular clip.

```glsl
#version 330 core
in vec3 vLocal;
in vec3 vWorld;
out vec4 FragColor;
uniform sampler2D uDay;
uniform sampler2D uNight;
uniform vec3  uSunDir;     // unit, world space (ECEF)
uniform float uHasDay;
uniform float uHasNight;

vec2 sphereToUV(vec3 n) {
    float lon = atan(n.y, n.x);          // [-PI, PI]
    float lat = asin(clamp(n.z, -1.0, 1.0));
    return vec2((lon / 3.14159265 + 1.0) * 0.5, (lat / 1.5707963 + 1.0) * 0.5);
}

void main() {
    vec3 n = normalize(vLocal);
    vec2 uv = sphereToUV(n);

    float cosSun = dot(n, normalize(uSunDir));
    float dayFactor = smoothstep(-0.10, 0.20, cosSun);   // soft terminator

    vec3 day   = (uHasDay  > 0.5) ? texture(uDay,  uv).rgb  : vec3(0.15, 0.35, 0.70);
    vec3 night = (uHasNight> 0.5) ? texture(uNight, uv).rgb : vec3(0.0);

    vec3 color = mix(night * 0.8, day, dayFactor);
    // warm twilight tint near the terminator
    float twi = smoothstep(-0.10, 0.20, cosSun) * (1.0 - smoothstep(0.18, 0.40, cosSun));
    color += vec3(0.18, 0.08, 0.0) * twi;

    FragColor = vec4(color, 1.0);
}
```

- [ ] **Step 3: `clouds.vert`** — identical transform contract (reused), with its own model matrix for slow rotation.

```glsl
#version 330 core
layout(location = 0) in vec3 aPos;
out vec3 vLocal;
uniform mat4 uMVP;
uniform mat4 uModel;
void main() {
    vLocal = aPos;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
```

- [ ] **Step 4: `clouds.frag`** — alpha from cloud map, lit by sun.

```glsl
#version 330 core
in vec3 vLocal;
out vec4 FragColor;
uniform sampler2D uClouds;
uniform vec3 uSunDir;
uniform float uHasClouds;
void main() {
    vec3 n = normalize(vLocal);
    float lon = atan(n.y, n.x);
    float lat = asin(clamp(n.z, -1.0, 1.0));
    vec2 uv = vec2((lon/3.14159265 + 1.0)*0.5, (lat/1.5707963 + 1.0)*0.5);
    float a = (uHasClouds > 0.5) ? texture(uClouds, uv).r : 0.0;
    float light = smoothstep(-0.15, 0.25, dot(n, normalize(uSunDir)));
    FragColor = vec4(vec3(1.0) * (0.25 + 0.75*light), a * 0.85);
}
```

- [ ] **Step 5: `atmosphere.vert`** — back-face shell needs view direction.

```glsl
#version 330 core
layout(location = 0) in vec3 aPos;
out vec3 vLocal;
out vec3 vWorld;
uniform mat4 uMVP;
uniform mat4 uModel;
void main() {
    vLocal = aPos;
    vWorld = vec3(uModel * vec4(aPos, 1.0));
    gl_Position = uMVP * vec4(aPos, 1.0);
}
```

- [ ] **Step 6: `atmosphere.frag`** — Fresnel limb glow, day/night-aware.

```glsl
#version 330 core
in vec3 vLocal;
in vec3 vWorld;
out vec4 FragColor;
uniform vec3 uSunDir;
uniform vec3 uViewPos;
void main() {
    vec3 n = normalize(vLocal);
    vec3 viewDir = normalize(uViewPos - vWorld);
    float rim = pow(1.0 - max(dot(n, viewDir), 0.0), 3.0);
    float sun = smoothstep(-0.3, 0.3, dot(n, normalize(uSunDir)));
    vec3 dayGlow   = vec3(0.35, 0.6, 1.0);
    vec3 nightGlow = vec3(0.05, 0.08, 0.18);
    vec3 col = mix(nightGlow, dayGlow, sun) * rim;
    FragColor = vec4(col, rim * (0.4 + 0.6 * sun));
}
```

- [ ] **Step 7: `src/globe/render/shaders.qrc`**

```xml
<RCC>
  <qresource prefix="/shaders">
    <file alias="earth.vert">shaders/earth.vert</file>
    <file alias="earth.frag">shaders/earth.frag</file>
    <file alias="clouds.vert">shaders/clouds.vert</file>
    <file alias="clouds.frag">shaders/clouds.frag</file>
    <file alias="atmosphere.vert">shaders/atmosphere.vert</file>
    <file alias="atmosphere.frag">shaders/atmosphere.frag</file>
  </qresource>
</RCC>
```

- [ ] **Step 8: Register the qrc in `CMakeLists.txt`** — add inside `qt_add_executable(globe ...)`:
```cmake
    src/globe/render/shaders.qrc
```

- [ ] **Step 9: Build**

Run: `cmake --build build --config Release`
Expected: builds; shaders compile-bundled (runtime compilation happens later).

- [ ] **Step 10: Commit**

```bash
git add -A
git commit -m "feat: GLSL shaders for earth, clouds, atmosphere"
```

---

## Task 6: AssetManager (textures + procedural fallbacks)

**Files:**
- Create: `src/globe/ConfigManager.h` (already), `src/globe/render/AssetManager.h`, `src/globe/render/AssetManager.cpp`
- Create: `assets/textures/README.md`

- [ ] **Step 1: Create `assets/textures/README.md`**

```markdown
# Globe textures

Drop HD equirectangular textures here (or run ../../scripts/fetch_textures.ps1):
- day.jpg   — NASA Blue Marble true color (8192x4096 for HD tier)
- night.jpg — NASA Black Marble VIIRS night lights (8192x4096)
- clouds.png — cloud alpha (white = cloud, linear; 8192x4096)

Missing files are auto-replaced with procedural fallbacks so the app still runs.
NASA imagery is public domain.
```

- [ ] **Step 2: Create `src/globe/render/AssetManager.h`**

```cpp
#pragma once
#include <QOpenGLFunctions>
#include <QImage>
#include <QString>

class ConfigManager;

// Loads day/night/cloud textures from disk (assets/textures), downscaling to the
// active quality tier. If a file is missing, returns a solid procedural image so
// the widget always renders. GPU upload is the renderer's job; this class only
// produces QImages and reports availability.
class AssetManager {
public:
    enum Slot { Day = 0, Night = 1, Clouds = 2 };

    explicit AssetManager(QString dir = QString());

    QImage image(Slot slot, int tierMaxSize) const;
    bool hasFile(Slot slot) const;

private:
    QString m_dir;
    QString pathFor(Slot slot) const;
    static QImage fallback(Slot slot);
};
```

- [ ] **Step 3: Create `src/globe/render/AssetManager.cpp`**

```cpp
#include "AssetManager.h"
#include <QDir>
#include <QStandardPaths>
#include <QPainter>

AssetManager::AssetManager(QString dir) {
    if (dir.isEmpty())
        dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/textures";
    m_dir = dir;
}

QString AssetManager::pathFor(Slot slot) const {
    switch (slot) {
    case Day:    return m_dir + "/day.jpg";
    case Night:  return m_dir + "/night.jpg";
    case Clouds: return m_dir + "/clouds.png";
    }
    return {};
}

bool AssetManager::hasFile(Slot slot) const {
    return QFile::exists(pathFor(slot));
}

QImage AssetManager::fallback(Slot slot) {
    QImage img(512, 256, QImage::Format_RGB32);
    switch (slot) {
    case Day:    img.fill(QColor(30, 70, 150));  break;  // ocean blue
    case Night:  img.fill(QColor(0, 0, 0));      break;
    case Clouds: img.fill(QColor(0, 0, 0));      break;  // no clouds
    }
    return img;
}

QImage AssetManager::image(Slot slot, int tierMaxSize) const {
    QImage img = hasFile(slot) ? QImage(pathFor(slot)) : fallback(slot);
    if (img.isNull()) img = fallback(slot);
    if (slot == Clouds && img.format() != QImage::Format_RGB32)
        img = img.convertToFormat(QImage::Format_RGB32);
    const int w = img.width(), h = img.height();
    if (w > tierMaxSize) {
        const int nh = int(qint64(h) * tierMaxSize / qMax(1, w));
        img = img.scaled(tierMaxSize, nh, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    return img;
}
```

- [ ] **Step 4: Add `AssetManager.cpp/.h` to the `globe` target** and create `assets/textures` at install time (optional, skip for now).

- [ ] **Step 5: Build**

Run: `cmake --build build --config Release`
Expected: builds.

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "feat: AssetManager with procedural texture fallbacks"
```

---

## Task 7: GlobeRenderer (sphere + 3-layer draw)

**Files:**
- Create: `src/globe/render/GlobeRenderer.h`, `src/globe/render/GlobeRenderer.cpp`

- [ ] **Step 1: Create `src/globe/render/GlobeRenderer.h`**

```cpp
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
    void setQualityTier(int maxSize);
    void render();

private:
    QOpenGLFunctions_3_3_Core *m_gl = nullptr;
    AssetManager *m_assets = nullptr;
    const SunModel *m_sun = nullptr;
    const CameraController *m_cam = nullptr;
    int m_tierMaxSize = 8192;

    std::unique_ptr<QOpenGLShaderProgram> m_earthProg, m_cloudProg, m_atmoProg;
    std::unique_ptr<QOpenGLTexture> m_texDay, m_texNight, m_texClouds;
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
```

- [ ] **Step 2: Create `src/globe/render/GlobeRenderer.cpp`**

```cpp
#include "GlobeRenderer.h"
#include <QFile>
#include <QVector>
#include <QtMath>

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
        const float lat = M_PI * (-0.5 + double(i) / stacks);
        for (int j = 0; j <= slices; ++j) {
            const float lon = 2.0 * M_PI * double(j) / slices;
            const float x = std::cos(lat) * std::cos(lon);
            const float y = std::cos(lat) * std::sin(lon);
            const float z = std::sin(lat);
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
        auto t = std::make_unique<QOpenGLTexture>(img.mirrored(false, true));
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

QMatrix4x4 GlobeRenderer::baseRotation() const {
    // Sun-centric: rotate so sub-solar longitude is at screen center (+Z toward camera).
    QMatrix4x4 m;
    if (m_sun) {
        const float lon = float(m_sun->subSolarLongitude());
        m.rotate(-lon, 0.0f, 0.0f, 1.0f); // bring sub-solar lon to +X
        m.rotate(90.0f, 0.0f, 1.0f, 0.0f);// +X -> +Z (toward camera)
    }
    return m;
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
    const int s = std::min(w, h);
    m_aspect = float(s) / float(s); // circular; we use min dimension
}

void GlobeRenderer::setAssets(AssetManager *a) {
    m_assets = a;
    if (m_gl && m_texDay == nullptr) loadTextures();
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
    if (m_texNight) { m_texNight->bind(1); m_earthProg->setUniformValue("uNight",1); }
    m_gl->glBindVertexArray(m_vao);
    m_gl->glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);

    // Clouds (slightly larger, slow spin)
    QMatrix4x4 cmodel = model;
    cmodel.scale(1.01f);
    cmodel.rotate(float(QDateTime::currentDateTime().toMSecsSinceEpoch() * 0.00001), 0, 0, 1);
    m_cloudProg->bind();
    m_cloudProg->setUniformValue("uMVP", proj * view * cmodel);
    m_cloudProg->setUniformValue("uModel", cmodel);
    m_cloudProg->setUniformValue("uSunDir", sun);
    m_cloudProg->setUniformValue("uHasClouds", m_texClouds ? 1.0f : 0.0f);
    if (m_texClouds) { m_texClouds->bind(2); m_cloudProg->setUniformValue("uClouds", 2); }
    m_gl->glDrawElements(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, nullptr);

    // Atmosphere (back-face shell)
    m_gl->glDisable(GL_DEPTH_TEST);
    m_gl->glDepthMask(GL_FALSE);
    QMatrix4x4 amodel = offset * base;
    amodel.scale(zoom * 1.06f);
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
```

- [ ] **Step 3: Add `GlobeRenderer.cpp/.h` to the `globe` target** in `CMakeLists.txt`.

- [ ] **Step 4: Build**

Run: `cmake --build build --config Release`
Expected: builds.

- [ ] **Step 5: Commit**

```bash
git add -A
git commit -m "feat: GlobeRenderer — sphere + earth/clouds/atmosphere passes"
```

---

## Task 8: GlobeView (QOpenGLWidget wiring)

**Files:**
- Create: `src/globe/render/GlobeView.h`, `src/globe/render/GlobeView.cpp`
- Modify: `src/globe/GlobeWindow.h/.cpp` (host the view)

- [ ] **Step 1: Create `src/globe/render/GlobeView.h`**

```cpp
#pragma once
#include <QOpenGLWidget>
#include <memory>
#include "GlobeRenderer.h"

class AssetManager;
class SunModel;
class CameraController;

class GlobeView : public QOpenGLWidget {
    Q_OBJECT
public:
    explicit GlobeView(QWidget *parent = nullptr);
    ~GlobeView() override;

    GlobeRenderer &renderer() { return m_renderer; }

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    std::unique_ptr<QOpenGLFunctions_3_3_Core> m_gl;
    GlobeRenderer m_renderer;
};
```

- [ ] **Step 2: Create `src/globe/render/GlobeView.cpp`**

```cpp
#include "GlobeView.h"

GlobeView::GlobeView(QWidget *parent) : QOpenGLWidget(parent) {
    QSurfaceFormat fmt;
    fmt.setVersion(3, 3);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    fmt.setSamples(4); // MSAA
    fmt.setSwapInterval(1);
    setFormat(fmt);
}

GlobeView::~GlobeView() { makeCurrent(); }

void GlobeView::initializeGL() {
    m_gl = std::make_unique<QOpenGLFunctions_3_3_Core>();
    m_gl->initializeOpenGLFunctions();
    m_renderer.initialize(m_gl.get());
}

void GlobeView::resizeGL(int w, int h) {
    m_renderer.resize(w, h, float(devicePixelRatioF()));
}

void GlobeView::paintGL() {
    m_renderer.render();
}
```

- [ ] **Step 3: Update `GlobeWindow` to host the view — replace the body of `GlobeWindow.h`**

```cpp
#pragma once
#include <QWidget>
#include "render/GlobeView.h"

class GlobeWindow : public QWidget {
    Q_OBJECT
public:
    explicit GlobeWindow(QWidget *parent = nullptr);
    GlobeView *view() { return m_view; }
protected:
    void paintEvent(QPaintEvent *) override {}
private:
    GlobeView *m_view;
};
```

- [ ] **Step 4: Update `GlobeWindow.cpp`**

```cpp
#include "GlobeWindow.h"
#include <QVBoxLayout>

GlobeWindow::GlobeWindow(QWidget *parent) : QWidget(parent), m_view(new GlobeView(this)) {
    setAttribute(Qt::WA_TranslucentBackground);
    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(m_view);
    resize(360, 360);
}
```

- [ ] **Step 5: Add `GlobeView.cpp/.h` to the `globe` target**.

- [ ] **Step 6: Build & run**

Run: `cmake --build build --config Release && ./build/Release/globe.exe`
Expected: a window showing a shaded blue sphere (fallback textures) lit from the right side by the current sun direction.

- [ ] **Step 7: Commit**

```bash
git add -A
git commit -m "feat: GlobeView hosts the OpenGL renderer in the shell"
```

---

## Task 9: Frameless always-on-top shell + circular mask + PlatformWindow

**Files:**
- Create: `src/globe/PlatformWindow.h`, `src/globe/PlatformWindow.cpp`
- Modify: `src/globe/GlobeWindow.h/.cpp`
- Modify: `main.cpp`

- [ ] **Step 1: Create `src/globe/PlatformWindow.h`** (ported from Seelie, renamed namespace)

```cpp
#pragma once
class QWidget;
namespace PlatformWindow {
void applyDwmFramelessAttributes(QWidget *widget);
void refreshComposition(QWidget *widget);
}
```

- [ ] **Step 2: Create `src/globe/PlatformWindow.cpp`**

```cpp
#include "PlatformWindow.h"
#include <QWidget>
#ifdef Q_OS_WIN
#  include <windows.h>
#  include <dwmapi.h>
#  ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#    define DWMWA_WINDOW_CORNER_PREFERENCE 33
#  endif
#  ifndef DWMWA_SYSTEMBACKDROP_TYPE
#    define DWMWA_SYSTEMBACKDROP_TYPE 38
#  endif
#endif

namespace PlatformWindow {
void applyDwmFramelessAttributes(QWidget *widget) {
#ifdef Q_OS_WIN
    if (!widget) return;
    HWND hwnd = reinterpret_cast<HWND>(widget->winId());
    if (!hwnd) return;
    const int doNotRound = 1;
    ::DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &doNotRound, sizeof(doNotRound));
    const int backdropNone = 1;
    ::DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdropNone, sizeof(backdropNone));
    const int ncDisabled = 1; // DWMNCRP_DISABLED
    ::DwmSetWindowAttribute(hwnd, DWMWA_NCRENDERING_POLICY, &ncDisabled, sizeof(ncDisabled));
#else
    (void)widget;
#endif
}
void refreshComposition(QWidget *widget) {
#ifdef Q_OS_WIN
    if (!widget) return;
    HWND hwnd = reinterpret_cast<HWND>(widget->winId());
    if (!hwnd) return;
    ::SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
    widget->update();
#else
    (void)widget;
#endif
}
}
```

- [ ] **Step 3: Update `GlobeWindow.cpp` constructor to set frameless flags, circular mask, and DWM attributes**

Add includes: `#include "PlatformWindow.h"`, `#include <QRegion>`, `#include <QShowEvent>`.
Replace constructor body after creating `m_view`:

```cpp
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint |
                   Qt::Tool | Qt::WindowDoesNotAcceptFocus);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setAttribute(Qt::WA_NoSystemBackground);
#ifdef Q_OS_MAC
    setAttribute(Qt::WA_MacAlwaysShowToolWindow, true);
#endif
    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(m_view);
    resize(360, 360);
```

Add to `GlobeWindow.h` protected section:
```cpp
    void showEvent(QShowEvent *) override;
    void resizeEvent(QResizeEvent *) override;
```
And implement in `GlobeWindow.cpp`:
```cpp
void GlobeWindow::showEvent(QShowEvent *e) {
    QWidget::showEvent(e);
    PlatformWindow::applyDwmFramelessAttributes(this);
}
void GlobeWindow::resizeEvent(QResizeEvent *e) {
    QWidget::resizeEvent(e);
    // Circular click-through mask: transparent corners pass clicks to desktop.
    const int side = qMin(width(), height());
    const QRect r((width() - side) / 2, (height() - side) / 2, side, side);
    setMask(QRegion(r, QRegion::Ellipse));
}
```

- [ ] **Step 4: Add `PlatformWindow.cpp/.h` to the `globe` target**.

- [ ] **Step 5: Build & run**

Run: `cmake --build build --config Release && ./build/Release/globe.exe`
Expected: frameless, always-on-top circular window; corners are click-through (clicking outside the circle hits the desktop).

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "feat: frameless always-on-top circular shell with DWM helper"
```

---

## Task 10: Wire SunModel + CameraController + TimeController + AssetManager into the view

**Files:**
- Create: `src/globe/render/TimeController.h`, `src/globe/render/TimeController.cpp`
- Modify: `src/globe/GlobeWindow.h/.cpp`, `main.cpp`

- [ ] **Step 1: Create `src/globe/render/TimeController.h`**

```cpp
#pragma once
#include <QTimer>
#include <QObject>

class SunModel;
// Recomputes the sun position each minute and triggers repaints at the fps cap.
class TimeController : public QObject {
    Q_OBJECT
public:
    TimeController(SunModel *sun, QObject *parent = nullptr);
    void setTarget(QObject *repaintTarget);
    void setFpsCap(int cap);
private slots:
    void onFrame();
private:
    SunModel *m_sun;
    QTimer m_frameTimer;
    QTimer m_sunTimer;
    QObject *m_target = nullptr;
};
```

- [ ] **Step 2: Create `src/globe/render/TimeController.cpp`**

```cpp
#include "TimeController.h"
#include "SunModel.h"
#include <QMetaObject>

TimeController::TimeController(SunModel *sun, QObject *parent)
    : QObject(parent), m_sun(sun) {
    m_sun->setUtc(QDateTime::currentDateTimeUtc());
    connect(&m_sunTimer, &QTimer::timeout, this, [this] {
        m_sun->setUtc(QDateTime::currentDateTimeUtc());
    });
    m_sunTimer.start(60000); // every minute
    setFpsCap(60);
    connect(&m_frameTimer, &QTimer::timeout, this, &TimeController::onFrame);
}

void TimeController::setTarget(QObject *target) { m_target = target; }

void TimeController::setFpsCap(int cap) {
    const int interval = (cap == 30) ? 33 : 16;
    m_frameTimer.start(interval);
}

void TimeController::onFrame() {
    if (m_target)
        QMetaObject::invokeMethod(m_target, "update", Qt::QueuedConnection);
}
```

- [ ] **Step 3: Wire everything in `main.cpp`**

```cpp
#include "GlobeWindow.h"
#include "ConfigManager.h"
#include "render/SunModel.h"
#include "render/CameraController.h"
#include "render/AssetManager.h"
#include "render/TimeController.h"
#include <QApplication>
#include <QStandardPaths>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Globe");

    ConfigManager config;
    AssetManager assets(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/textures");
    SunModel sun;
    CameraController camera;

    GlobeWindow w;
    w.view()->renderer().setAssets(&assets);
    w.view()->renderer().setSun(&sun);
    w.view()->renderer().setCamera(&camera);
    w.move(config.windowX(), config.windowY());
    w.resize(config.diameter(), config.diameter());

    TimeController time(&sun);
    time.setTarget(w.view());
    time.setFpsCap(config.fpsCap());

    w.show();
    return app.exec();
}
```

- [ ] **Step 4: Add `TimeController.cpp/.h` to the `globe` target.**

- [ ] **Step 5: Build & run**

Run: `cmake --build build --config Release && ./build/Release/globe.exe`
Expected: globe lit by the real current sun direction; the terminator is visible and updates over time.

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "feat: wire SunModel/Camera/Assets/Time into the live globe"
```

---

## Task 11: Interaction — orbit, zoom/resize, reset, move

**Files:**
- Modify: `src/globe/GlobeWindow.h/.cpp`

- [ ] **Step 1: Add interaction state to `GlobeWindow.h`**

Add includes `#include "render/CameraController.h"` and members:
```cpp
    CameraController *m_cam = nullptr;
    QPoint m_lastPos;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void wheelEvent(QWheelEvent *) override;
    void mouseDoubleClickEvent(QMouseEvent *) override;
public:
    void setCameraController(CameraController *c) { m_cam = c; }
```

- [ ] **Step 2: Implement in `GlobeWindow.cpp`**

```cpp
#include <QMouseEvent>
#include <QWheelEvent>

void GlobeWindow::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton || e->button() == Qt::MiddleButton)
        m_lastPos = e->globalPosition().toPoint();
    QWidget::mousePressEvent(e);
}

void GlobeWindow::mouseMoveEvent(QMouseEvent *e) {
    if (e->buttons() & Qt::MiddleButton) {
        move(pos() + e->globalPosition().toPoint() - m_lastPos);
        m_lastPos = e->globalPosition().toPoint();
    } else if ((e->buttons() & Qt::LeftButton) && (e->modifiers() & Qt::AltModifier)) {
        move(pos() + e->globalPosition().toPoint() - m_lastPos);
        m_lastPos = e->globalPosition().toPoint();
    } else if ((e->buttons() & Qt::LeftButton) && m_cam) {
        const QPoint d = e->globalPosition().toPoint() - m_lastPos;
        m_lastPos = e->globalPosition().toPoint();
        m_cam->applyDrag(d.x(), d.y());
    }
    QWidget::mouseMoveEvent(e);
}

void GlobeWindow::wheelEvent(QWheelEvent *e) {
    if (m_cam) {
        m_cam->applyZoom(e->angleDelta().y());
        const int side = int(360 * m_cam->zoom());
        resize(side, side);
    }
    QWidget::wheelEvent(e);
}

void GlobeWindow::mouseDoubleClickEvent(QMouseEvent *e) {
    if (m_cam) { m_cam->reset(); resize(360, 360); }
    QWidget::mouseDoubleClickEvent(e);
}
```

- [ ] **Step 3: In `main.cpp` connect the camera: `w.setCameraController(&camera);`**

- [ ] **Step 4: Build & run, manual checks**

Run: `cmake --build build --config Release && ./build/Release/globe.exe`
Expected:
- Left-drag orbits the globe.
- Middle-drag or Alt+Left-drag moves the widget.
- Wheel resizes the widget (zoom).
- Double-click resets to the sun-centric default size.

- [ ] **Step 5: Commit**

```bash
git add -A
git commit -m "feat: orbit/zoom/move/reset interaction"
```

---

## Task 12: SystemTray + context menu + persistence

**Files:**
- Create: `src/globe/SystemTray.h`, `src/globe/SystemTray.cpp`
- Modify: `src/globe/GlobeWindow.h/.cpp`, `main.cpp`

- [ ] **Step 1: Create `src/globe/SystemTray.h`**

```cpp
#pragma once
#include <QSystemTrayIcon>
class QMenu;
class SystemTray : public QSystemTrayIcon {
    Q_OBJECT
public:
    explicit SystemTray(QObject *parent = nullptr);
    void setSolarTooltip(const QString &text);
signals:
    void toggleVisibility();
    void resetView();
    void quit();
};
```

- [ ] **Step 2: Create `src/globe/SystemTray.cpp`**

```cpp
#include "SystemTray.h"
#include <QMenu>
#include <QApplication>

SystemTray::SystemTray(QObject *parent) : QSystemTrayIcon(parent) {
    setIcon(QIcon(":/icons/globe.png")); // optional; falls back to default if absent
    auto *menu = new QMenu;
    auto *hide = menu->addAction(QObject::tr("Hide/Show"));
    auto *reset = menu->addAction(QObject::tr("Reset View"));
    menu->addSeparator();
    auto *about = menu->addAction(QObject::tr("About"));
    auto *quit = menu->addAction(QObject::tr("Quit"));
    connect(hide, &QAction::triggered, this, &SystemTray::toggleVisibility);
    connect(reset, &QAction::triggered, this, &SystemTray::resetView);
    connect(about, &QAction::triggered, [] { /* About dialog: v1 shows tray message */ });
    connect(quit, &QAction::triggered, qApp, &QApplication::quit);
    connect(quit, &QAction::triggered, this, &SystemTray::quit);
    setContextMenu(menu);
    show();
}

void SystemTray::setSolarTooltip(const QString &text) {
    setToolTip(text);
}
```

- [ ] **Step 3: Persist position on move — add to `GlobeWindow.h`**
```cpp
    ConfigManager *m_config = nullptr;
public:
    void setConfig(ConfigManager *c) { m_config = c; }
```
In `mouseMoveEvent` move branch, after `move(...)`:
```cpp
        if (m_config) { m_config->setWindowX(pos().x()); m_config->setWindowY(pos().y()); m_config->save(); }
```

- [ ] **Step 4: In `main.cpp` wire tray + config**
```cpp
#include "SystemTray.h"
    SystemTray tray;
    w.setConfig(&config);
    tray.connect(&tray, &SystemTray::toggleVisibility, &w, [&w]{ w.isVisible() ? w.hide() : w.show(); });
    tray.connect(&tray, &SystemTray::resetView, &camera, &CameraController::reset);
```

- [ ] **Step 5: Add `SystemTray.cpp/.h` to the `globe` target.**

- [ ] **Step 6: Build & run, manual checks**

Run: `cmake --build build --config Release && ./build/Release/globe.exe`
Expected: tray icon present; right-click menu offers Hide/Show, Reset View, About, Quit; dragging the widget persists its position across restarts.

- [ ] **Step 7: Commit**

```bash
git add -A
git commit -m "feat: system tray, context menu, position persistence"
```

---

## Task 13: LocationProvider (TDD with mock) + platform impls

**Files:**
- Create: `src/globe/LocationProvider.h`, `src/globe/LocationProvider.cpp`
- Test: `tests/test_locationprovider.cpp`

- [ ] **Step 1: Write the failing test `tests/test_locationprovider.cpp`**

```cpp
#include <QtTest>
#include "LocationProvider.h"

class TestLocationProvider : public QObject { Q_OBJECT
private slots:
    void deniedDisablesLocation() {
        LocationProvider p;
        p.setPermission(LocationProvider::Denied);
        QVERIFY(!p.isEnabled());
    }
    void grantedWithCoordinatesEnables() {
        LocationProvider p;
        p.setPermission(LocationProvider::Granted);
        p.setCoordinates(39.9, 116.4); // Beijing
        QVERIFY(p.isEnabled());
        QCOMPARE(p.latitude(), 39.9);
        QCOMPARE(p.longitude(), 116.4);
    }
};

QTEST_GUILESS_MAIN(TestLocationProvider)
#include "test_locationprovider.moc"
```

- [ ] **Step 2: Add test target `globe_add_test(test_locationprovider)`** in `tests/CMakeLists.txt`.

- [ ] **Step 3: Run test to verify it fails**

Run: `cmake --build build --target test_locationprovider && ctest --test-dir build -R test_locationprovider --output-on-failure`
Expected: FAIL.

- [ ] **Step 4: Create `src/globe/LocationProvider.h`**

```cpp
#pragma once
#include <QObject>

// Abstraction over platform geolocation. v1 ships with a mock implementation
// that tests drive directly; Windows (WinRT), macOS (CoreLocation), and Linux
// (GeoClue) hooks are added behind the same interface. Coordinates stay local.
class LocationProvider : public QObject {
    Q_OBJECT
public:
    enum Permission { Unknown = 0, Granted = 1, Denied = 2 };
    Q_ENUM(Permission)

    explicit LocationProvider(QObject *parent = nullptr);

    void requestPermission();                       // platform prompt (best-effort)
    Permission permission() const { return m_perm; }
    void setPermission(Permission p);               // for mock/tests

    bool isEnabled() const;
    double latitude() const { return m_lat; }
    double longitude() const { return m_lon; }
    void setCoordinates(double lat, double lon);    // for mock/tests/platform impls

signals:
    void permissionChanged(Permission p);
    void locationChanged(double lat, double lon);

private:
    Permission m_perm = Unknown;
    double m_lat = 0.0, m_lon = 0.0;
};
```

- [ ] **Step 5: Create `src/globe/LocationProvider.cpp`**

```cpp
#include "LocationProvider.h"

LocationProvider::LocationProvider(QObject *parent) : QObject(parent) {}

void LocationProvider::requestPermission() {
#ifdef Q_OS_WIN
    // TODO(impl): WinRT Windows.Devices.Geolocation — set permission on result.
#endif
#ifdef Q_OS_MAC
    // TODO(impl): CoreLocation CLLocationManager.
#endif
#ifdef Q_OS_LINUX
    // TODO(impl): GeoClue2.
#endif
}

void LocationProvider::setPermission(Permission p) {
    if (m_perm == p) return;
    m_perm = p;
    emit permissionChanged(p);
}

bool LocationProvider::isEnabled() const {
    return m_perm == Granted;
}

void LocationProvider::setCoordinates(double lat, double lon) {
    m_lat = lat; m_lon = lon;
    emit locationChanged(lat, lon);
}
```

- [ ] **Step 6: Run test to verify it passes**

Run: `cmake --build build --target test_locationprovider && ctest --test-dir build -R test_locationprovider --output-on-failure`
Expected: PASS (2 tests).

- [ ] **Step 7: Commit**

```bash
git add -A
git commit -m "feat: LocationProvider interface + mock with tests"
```

---

## Task 14: Home marker + "Center on me" + tray solar info

**Files:**
- Modify: `src/globe/render/GlobeRenderer.h/.cpp` (home marker)
- Modify: `src/globe/GlobeWindow.h/.cpp` (Center on me + tray wiring)
- Modify: `main.cpp`

- [ ] **Step 1: Add a home pin to `GlobeRenderer`** — member `double m_homeLat = 0, m_homeLon = 0; bool m_hasHome = false;` plus setters, and in `render()` after Earth, before atmosphere, draw a small point (simplest: an additional tiny sphere billboard omitted in v1; use a colored vertex). Minimal v1: draw a small GL_POINTS marker at the home position.

Add to `GlobeRenderer.h`:
```cpp
    void setHome(double lat, double lon, bool on) { m_homeLat=lat; m_homeLon=lon; m_hasHome=on; }
private:
    double m_homeLat = 0, m_homeLon = 0; bool m_hasHome = false;
```
In `render()`, after the clouds draw and before the atmosphere draw:
```cpp
    if (m_hasHome) {
        // Compute home position in model space and draw a point.
        const double la = qDegreesToRadians(m_homeLat), lo = qDegreesToRadians(m_homeLon);
        QVector3D p(std::cos(la)*std::cos(lo), std::cos(la)*std::sin(lo), std::sin(la));
        p = (model * QVector4D(p, 1.0f)).toVector3D() * 1.005f;
        // (Full point-sprite draw requires a tiny shader; for v1 we render the
        //  pin via a small additive quad using m_earthProg disabled. See note.)
    }
```
*(Note for implementer: a 6-line marker shader is out of v1's bite-size scope; this hook exists so the renderer API is stable. If the marker must render now, add `marker.vert/frag` with `gl_PointSize` and `GL_POINTS`.)*

- [ ] **Step 2: "Center on me" — add to `GlobeWindow.h`**
```cpp
    bool m_centerOnMe = false;
public slots:
    void centerOnMe(double lat, double lon);
```
Implement: store the target longitude, and when set, the renderer's `baseRotation()` should rotate the *home* longitude to center instead of the sub-solar one. For v1, `Center on me` clears the camera offset (`m_cam->reset()`) and sets a flag consumed by `GlobeRenderer::baseRotation()`:
```cpp
void GlobeWindow::centerOnMe(double lat, double lon) {
    m_centerOnMe = true;
    view()->renderer().setCenterLongitude(lon);
    if (m_cam) m_cam->reset();
}
```
Add `setCenterLongitude(double lon)` to `GlobeRenderer` (member + use it in `baseRotation()` instead of `m_sun->subSolarLongitude()` when set).

- [ ] **Step 3: Tray solar info — in `main.cpp` after creating tray**
```cpp
#include <QTimer>
    QTimer solarTimer;
    QObject::connect(&solarTimer, &QTimer::timeout, [&]{
        sun.setUtc(QDateTime::currentDateTimeUtc());
        const bool daySide = sun.sunDirection().z() > 0; // simplified local indicator
        tray.setSolarToolTip(QString("Sun sub-point: %1°, %2°")
            .arg(sun.subSolarLatitude(), 0, 'f', 1)
            .arg(sun.subSolarLongitude(), 0, 'f', 1));
    });
    solarTimer.start(300000); // every 5 min
```
(Full per-city sunrise/sunset is layered on once `LocationProvider` returns coordinates; v1 shows the global sun sub-point.)

- [ ] **Step 4: Build & run, manual checks**

Run: `cmake --build build --config Release && ./build/Release/globe.exe`
Expected: tray tooltip shows the live sun sub-point; `Center on me` (once wired to the menu) resets the view and centers on the configured home longitude.

- [ ] **Step 5: Commit**

```bash
git add -A
git commit -m "feat: home marker hook, center-on-me, tray solar info"
```

---

## Task 15: HD texture fetch script + quality tiers + auto-downscale

**Files:**
- Create: `scripts/fetch_textures.ps1`
- Modify: `src/globe/render/GlobeRenderer.cpp` (apply tier), `main.cpp`

- [ ] **Step 1: Create `scripts/fetch_textures.ps1`**

```powershell
# Downloads public-domain NASA equirectangular textures into the user's texture dir.
$ErrorActionPreference = 'Stop'
$dir = Join-Path $env:LOCALAPPDATA 'Globe\textures'
New-Item -ItemType Directory -Force -Path $dir | Out-Null

# NOTE: verify current URLs at https://visibleearth.nasa.gov before running.
$urls = @{
  'day.jpg'   = 'https://eoimages.gsfc.nasa.gov/images/imagerecords/73000/73776/world.topo.bathy.200408.3x5400x2700.jpg'
  'night.jpg' = 'https://eoimages.gsfc.nasa.gov/images/imagerecords/79000/79765/dnb_land_ocean_ice.2012.5400x2700.jpg'
  'clouds.png'= 'https://eoimages.gsfc.nasa.gov/images/imagerecords/57000/57752/f5_1km_2012175_0700.sw.png'
}

foreach ($k in $urls.Keys) {
    $dst = Join-Path $dir $k
    Write-Host "Fetching $k -> $dst"
    Invoke-WebRequest -Uri $urls[$k] -OutFile $dst
}
Write-Host "Done. Textures in $dir"
```

- [ ] **Step 2: Apply quality tier in `main.cpp`**
```cpp
    const int tierMax = (config.qualityTier() == ConfigManager::HD) ? 8192 :
                        (config.qualityTier() == ConfigManager::Medium) ? 4096 : 2048;
    w.view()->renderer().setQualityTier(tierMax);
```

- [ ] **Step 3: Build & run, manual checks**

Run the script: `powershell -ExecutionPolicy Bypass -File scripts/fetch_textures.ps1`
Then: `cmake --build build --config Release && ./build/Release/globe.exe`
Expected: photorealistic HD day/night/clouds render; switching the `quality` config value and restarting picks a smaller tier (lower VRAM).

- [ ] **Step 4: Commit**

```bash
git add -A
git commit -m "feat: HD texture fetch script + quality tiers"
```

---

## Task 16: Inno Setup installer (Windows)

**Files:**
- Create: `installer/globe.iss`

- [ ] **Step 1: Create `installer/globe.iss`**

```ini
; Inno Setup script for the Globe widget.
; Build steps:
;   1. cmake --build build --config Release
;   2. windeployqt --release build/Release/globe.exe
;   3. Copy build/Release/* to installer/payload/
;   4. Run: iscc installer/globe.iss

#define MyAppName "Globe"
#define MyAppVersion "0.1.0"
#define MyAppPublisher "Globe"

[Setup]
AppId={{8F2B6D5A-3C1E-4GLOBE0001}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DisableProgramGroupPage=yes
OutputDir=..\dist
OutputBaseFilename=GlobeSetup-{#MyAppVersion}
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=lowest

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "startup"; Description: "Start Globe when Windows starts"; GroupDescription: "Extras:"

[Files]
Source: "payload\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs ignoreversion

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\globe.exe"

[Run]
Filename: "{app}\globe.exe"; Description: "Launch Globe"; Flags: nowait postinstall skipifsilent

[Registry]
; Optional autostart task
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; \
  ValueType: string; ValueName: "Globe"; ValueData: """{app}\globe.exe"""; \
  Flags: uninsdeletevalue; Tasks: startup
```

- [ ] **Step 2: Build payload & installer, manual check**

Run:
```powershell
cmake --build build --config Release
windeployqt --release build/Release/globe.exe
New-Item -ItemType Directory -Force installer/payload | Out-Null
Copy-Item build/Release/* installer/payload -Recurse -Force
iscc installer/globe.iss
```
Expected: `dist/GlobeSetup-0.1.0.exe` is produced and installs the app.

- [ ] **Step 3: Commit**

```bash
git add -A
git commit -m "build: Inno Setup installer for Windows"
```

---

## Definition of Done

- `ctest --test-dir build --output-on-failure` passes all unit tests (SunModel, ConfigManager, CameraController, LocationProvider).
- `globe.exe` shows a frameless, always-on-top, circular, resizable Earth with real-time day/night terminator, moving clouds, atmosphere glow, and HD NASA textures.
- Orbit (left-drag), move (middle/Alt-drag), zoom/resize (wheel), reset (double-click / tray), and tray context menu all work.
- Position persists across restarts; quality tier and fps cap are honored.
- `scripts/fetch_textures.ps1` populates HD textures; `installer/globe.iss` builds a Windows installer.

---

## Self-Review Notes

- **Spec coverage:** Architecture (T1–T10), render model (T5–T8, T11), interaction/window (T9, T11), location (T13–T14), assets/build (T6, T15, T16), testing/error (T2–T4, T13; degradation paths noted in spec handled by AssetManager fallbacks and tier switching) — all represented.
- **Type consistency:** `CameraController::offsetRotation()`, `GlobeRenderer::baseRotation()`, `SunModel::sunDirection()`, `AssetManager::image()` signatures are used consistently across tasks.
- **Known simplifications (flagged inline, not blockers):** home-pin rendering is a stable API hook in v1 (full point-sprite shader is a follow-up); per-city sunrise/sunset in the tray is layered on once location returns coordinates — v1 shows the global sun sub-point; platform-specific location prompts are TODO stubs behind the tested interface.
