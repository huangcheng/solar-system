# Solar System Expansion — Design Spec

**Date:** 2026-06-19  
**Status:** Draft — awaiting implementation plan  
**Decision owner:** User approved Approach A (shared core + separate executables)  

## Goal

Expand the existing Qt6/OpenGL Earth desktop widget into a **celestial-body widget collection** plus a **unified Solar System simulator**.

- **Earth** becomes the first member of the collection.
- Every major solar-system body (Sun, Moon, Mercury, Venus, Mars, Jupiter, Saturn, Uranus, Neptune) gets its own standalone desktop widget executable.
- A new `solar` executable acts as both a **launcher/control panel** and a **Solar System Mode** that renders all bodies orbiting the Sun with a stars/Milky-Way background.
- Only one instance of each body is allowed at a time (singleton per body).
- Cross-platform: Windows, macOS, Linux.

## Non-Goals

- JPL-precision ephemerides. The simulation is visually correct, not mission-planning accurate.
- Multi-body gravitational physics (n-body). Planets follow fixed Keplerian orbits around the Sun; the Moon follows a simplified orbit around Earth.
- Spacecraft, asteroids, comets, dwarf planets, or exoplanets in the initial release.
- Network features, online texture streaming, or user-generated content.

## Architecture

### Approach

**Approach A: Shared Core Library + Separate Executables**

One CMake project builds a shared static library (`celestial`) and ten executable targets (`earth`, `sun`, `moon`, `mercury`, `venus`, `mars`, `jupiter`, `saturn`, `uranus`, `neptune`, `solar`).

### Project Layout

```
F:\globe\
├── CMakeLists.txt
├── src/
│   ├── celestial/                 # static library
│   │   ├── CelestialBody.h/.cpp   # generic sphere renderer
│   │   ├── BodyConfig.h/.cpp      # per-body defaults
│   │   ├── OrbitModel.h/.cpp      # Keplerian orbits + time scale
│   │   ├── SolarSystemScene.h/.cpp# unified solar system renderer
│   │   ├── AssetManager.h/.cpp    # textures, shaders, skybox
│   │   ├── ShaderManager.h/.cpp   # shared GLSL programs
│   │   ├── CameraController.h/.cpp# 3D camera
│   │   ├── CelestialWidget.h/.cpp # common desktop widget shell
│   │   ├── SettingsDialog.h/.cpp  # common settings UI framework
│   │   ├── ConfigManager.h/.cpp   # per-body user config
│   │   ├── FullscreenWatcher.h/.cpp
│   │   └── PlatformWindow.h/.cpp
│   └── apps/
│       ├── earth/main.cpp
│       ├── sun/main.cpp
│       ├── moon/main.cpp
│       ├── mercury/main.cpp
│       ├── venus/main.cpp
│       ├── mars/main.cpp
│       ├── jupiter/main.cpp
│       ├── saturn/main.cpp
│       ├── uranus/main.cpp
│       ├── neptune/main.cpp
│       └── solar/main.cpp
├── assets/
│   └── textures/
│       ├── 8k_earth.jpg
│       ├── 8k_sun.jpg
│       ├── 8k_moon.jpg
│       ├── 8k_mercury.jpg
│       ├── 4k_venus_atmosphere.jpg
│       ├── 8k_mars.jpg
│       ├── 8k_jupiter.jpg
│       ├── 8k_saturn.jpg
│       ├── 8k_saturn_ring_alpha.png
│       ├── 2k_uranus.jpg
│       ├── 2k_neptune.jpg
│       ├── 8k_stars.jpg
│       └── 8k_stars_milky_way.jpg
├── tests/
│   └── (per-component unit tests)
└── docs/superpowers/specs/2026-06-19-solar-system-design.md
```

### Existing Code Refactor

The current Earth-specific code moves into the shared library:

| Current | New Home |
|---------|----------|
| `GlobeRenderer` | `CelestialBody` |
| `GlobeView` | `CelestialWidget` |
| `CameraController` | `CameraController` (generalized) |
| `AssetManager` | `AssetManager` (multi-texture) |
| `TimeController` | `OrbitModel` + per-body spin |
| `SunModel` | `OrbitModel` |
| `GlobeWindow` | `CelestialWidget` |
| `ConfigManager` | `ConfigManager` (per-body keys) |
| `SettingsDialog` | `SettingsDialog` (body-specific pages) |
| `SystemTray` | `SystemTray` (common) |
| `FullscreenWatcher` | `FullscreenWatcher` (common) |

Earth's behavior is preserved exactly; only the namespace/class names change.

## Shared Core Components

### `BodyConfig`

Immutable defaults for a body:

```cpp
struct BodyConfig {
    QString bodyId;              // "earth", "sun", "saturn"
    QString displayName;         // tr("Earth")
    QString albedoTexture;       // "8k_earth.jpg"
    QString nightTexture;        // optional
    QString normalTexture;       // optional
    QString ringTexture;         // optional (Saturn)

    float radiusKm = 6371.0f;    // physical radius
    float visualScale = 1.0f;    // multiplier for widget/scene display
    float rotationPeriodHours;   // sidereal rotation
    float axialTiltDegrees;
    bool emissive = false;       // Sun
    bool hasRings = false;       // Saturn
    bool hasAtmosphere = false;  // Earth, Venus

    // Keplerian elements (heliocentric, for planets)
    float semiMajorAxisAU;
    float eccentricity;
    float inclinationDeg;
    float longitudeAscendingNodeDeg;
    float argumentOfPerihelionDeg;
    float meanAnomalyAtEpochDeg;
};
```

Factory functions provide canonical values:

```cpp
BodyConfig BodyConfig::earth();
BodyConfig BodyConfig::sun();
BodyConfig BodyConfig::saturn();
// ...
```

### `CelestialBody`

Generic sphere renderer:

```cpp
class CelestialBody {
public:
    void load(const BodyConfig& config, AssetManager& assets);
    void update(float dtSeconds, float timeScale);
    void render(const QMatrix4x4& viewProj,
                const QVector3D& sunDirectionWorld,
                const RenderOptions& opts);

    const BodyConfig& config() const;
    float currentRotationAngle() const;
};
```

The class selects the correct shader pipeline based on config flags:

| Body type | Shader behavior |
|-----------|-----------------|
| Rocky planet | Albedo + normal + day/night based on `sunDirection` |
| Gas giant | Albedo + band shading, optional storm features |
| Star | Emissive albedo + corona/glow post-effect |
| Ringed planet | Planet shader + transparent ring plane |
| Atmosphere | Thin rim light additive pass |

### `OrbitModel`

Computes heliocentric position from Keplerian elements and a simulated time.

```cpp
class OrbitModel {
public:
    OrbitModel(const BodyConfig& config);

    // Advances internal simulated time by dt * timeScale.
    void update(float dtSeconds);

    // Returns position in AU relative to the Sun.
    QVector3D position() const;

    // Simulated UTC date/time.
    QDateTime simulatedDateTime() const;

    float timeScale() const;
    void setTimeScale(float yearsPerMinute);
};
```

`OrbitModel` uses Newton-Raphson to solve Kepler's equation for eccentric anomaly, then converts to true anomaly and cartesian coordinates. The Moon uses a separate simplified Earth-centered orbit helper.

Default time scale: **1 Earth year = 60 seconds**.
Range: **1 Earth year = 10 seconds … 1 Earth year = 1 hour**.

### `SolarSystemScene`

Renders the unified Solar System Mode:

```cpp
class SolarSystemScene {
public:
    void load(AssetManager& assets);
    void update(float dtSeconds);
    void render(const CameraController& camera, const RenderOptions& opts);

    void setShowOrbits(bool);
    void setShowLabels(bool);
    void setShowMilkyWay(bool);
    void setTimeScale(float yearsPerMinute);
};
```

Render passes:

1. **Skybox**: cube map of `8k_stars_milky_way.jpg` or `8k_stars.jpg`.
2. **Orbit trails**: faint ellipses for each planet.
3. **Bodies**: Sun at origin; each planet at `OrbitModel::position()`, rotated by axial tilt and current spin.
4. **Rings**: Saturn's ring plane.
5. **Glow/bloom**: Sun corona, atmospheric rims.
6. **HUD**: labels, simulated date, time-scale slider.

### `CelestialWidget`

Common desktop widget shell used by every body executable:

- Frameless, circular mask (current `GlobeWindow` behavior)
- Default size 220 px; resizable by mouse wheel
- Draggable; always-on-top toggle
- Double-click resets size
- Right-click / tray icon opens body-specific settings
- Hides over fullscreen apps

```cpp
class CelestialWidget : public QWidget {
public:
    explicit CelestialWidget(const BodyConfig& config, ConfigManager* configManager);
    CelestialBody* body() const;
};
```

### `SettingsDialog`

A common settings dialog framework with body-specific pages. The base dialog contains:

- General page: language, always-on-top, show FPS, size
- Body page: injected by the executable based on `BodyConfig::bodyId`

Per-body settings examples:

| Body | Specific settings |
|------|-------------------|
| Earth | day/night mode, grid, spin speed, terrain relief |
| Sun | corona intensity, surface turbulence |
| Saturn | ring opacity, ring tilt |
| Jupiter | Great Red Spot visibility |
| Moon | libration toggle, phase lighting |

### `ConfigManager`

Persists user config per body in separate JSON files:

```
%LOCALAPPDATA%\Globe\earth-config.json
%LOCALAPPDATA%\Globe\sun-config.json
%LOCALAPPDATA%\Globe\solar-config.json
```

Config keys include `diameter`, `alwaysOnTop`, `language`, `rotationSpeed`, plus body-specific keys.

### Singleton Enforcement

Each executable checks a body-specific mutex at startup:

- `GlobeAppSingleton_Earth_v1`
- `GlobeAppSingleton_Sun_v1`
- `GlobeAppSingleton_Solar_v1`
- ...

On Windows: named mutex via Win32 API.  
On macOS/Linux: `QSharedMemory` with body-specific key.

## Executables

### Body Executables (`earth`, `sun`, `moon`, ...)

Each is ~30 lines:

```cpp
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Earth");

    if (!acquireSingleton("Earth")) return 0;

    ConfigManager config("earth");
    config.load();

    CelestialWidget widget(BodyConfig::earth(), &config);
    widget.show();

    return app.exec();
}
```

### `solar` — Launcher + Solar System Mode

`solar.exe` has two windows:

#### Launcher Panel

A small QWidget window listing all bodies:

- "Add to Desktop" buttons for each body (spawns the corresponding executable)
- "Solar System Mode" button (shows the unified scene)
- Global settings: texture quality, language, default time scale
- "Exit All" to terminate every running body widget

#### Solar System Mode

A large transparent window covering the desktop by default (toggle to windowed):

- Renders `SolarSystemScene`
- Camera: mouse drag to orbit, scroll to zoom, right-click menu
- Time-scale slider and play/pause button
- Simulated date display
- Toggle orbits, labels, Milky Way
- Button to return to launcher

## Rendering

### Shaders

Reuse the existing GLSL 3.3 pipeline but generalize it:

- `planet.vert` / `planet.frag` — albedo + normal + day/night
- `gasgiant.vert` / `gasgiant.frag` — banded sphere
- `star.vert` / `star.frag` — emissive + corona
- `ring.vert` / `ring.frag` — textured ring plane with transparency
- `skybox.vert` / `skybox.frag` — cube map background
- `orbit.vert` / `orbit.frag` — line-loop ellipse

### Assets

Textures are loaded from `assets/textures/` relative to the executable. On Windows deployment, the folder is copied next to the `.exe`. Each body uses the corresponding texture from the user's download directory (`C:\Users\huang\Downloads\textures`) as the canonical source during development.

## Build System

CMake target structure:

```cmake
add_library(celestial STATIC
    src/celestial/CelestialBody.cpp
    src/celestial/BodyConfig.cpp
    src/celestial/OrbitModel.cpp
    src/celestial/SolarSystemScene.cpp
    src/celestial/AssetManager.cpp
    src/celestial/ShaderManager.cpp
    src/celestial/CameraController.cpp
    src/celestial/CelestialWidget.cpp
    src/celestial/SettingsDialog.cpp
    src/celestial/ConfigManager.cpp
    src/celestial/FullscreenWatcher.cpp
    src/celestial/PlatformWindow.cpp
)

target_link_libraries(celestial PUBLIC Qt6::Widgets Qt6::OpenGLWidgets)

set(BODIES earth sun moon mercury venus mars jupiter saturn uranus neptune solar)
foreach(body ${BODIES})
    add_executable(${body} src/apps/${body}/main.cpp)
    target_link_libraries(${body} PRIVATE celestial)
endforeach()
```

## Cross-Platform Notes

- **Windows**: windeployqt for each executable, or one shared bin folder with all Qt DLLs.
- **macOS**: Bundle `.app` per executable; share `assets` via symlink or copy.
- **Linux**: AppImage or direct binary + `assets` folder.
- Singleton uses platform-specific APIs.

## MVP Phasing

Because this is a large refactor, implementation is split into phases. Each phase is buildable and testable on its own.

### Phase 1: Refactor Earth

- Create `src/celestial/` and move shared classes.
- Rename current `earth` rendering path to use `CelestialBody` + `CelestialWidget`.
- Build only `earth.exe` at first.
- Ensure current behavior (spin, day/night, grid, settings) is identical.
- Unit tests for `BodyConfig`, `OrbitModel`, `CelestialBody`.

### Phase 2: Add Sun and Moon

- Add `sun.exe` and `moon.exe`.
- Implement star and rocky-planet shaders.
- Verify singleton per body.

### Phase 3: Add Rocky and Gas Giants

- Add `mercury`, `venus`, `mars`, `jupiter`, `saturn`, `uranus`, `neptune`.
- Implement gas-giant and ring shaders.
- Each body gets its own settings page.

### Phase 4: Launcher

- Build `solar.exe` launcher panel.
- "Add to Desktop" spawns body executables.
- "Exit All" terminates all running instances.

### Phase 5: Solar System Mode

- Build `SolarSystemScene` with skybox, orbits, and all bodies.
- Add camera controls and time-scale slider.
- Add HUD (labels, date).

### Phase 6: Polish

- Orbit trails, labels toggle, Milky Way toggle.
- Performance tuning (LOD, texture streaming).
- Full translation support for all UI strings.

## Testing Strategy

- Unit tests for `OrbitModel` against known ephemeris values (Earth at J2000, etc.).
- Unit tests for `BodyConfig` factories.
- Visual smoke tests: each executable launches and renders without crash.
- Solar System Mode smoke test: `solar.exe --mode solar-system` renders and advances time.

## Open Questions Resolved

| Question | Decision |
|----------|----------|
| Separate executables? | Yes, one per body plus `solar` |
| `solar.exe` role? | Launcher panel + unified Solar System Mode |
| How to orbit? | Unified Solar System Mode renders all bodies in one scene |
| Time scale? | Configurable slider, default 1 year = 60 seconds |
| Architecture | Approach A: shared `celestial` library + thin executables |
| Accuracy | Keplerian elements, visually correct |
| Background | Stars/Milky Way skybox in Solar System Mode |

## Next Step

After this spec is approved, invoke the `writing-plans` skill to produce a detailed implementation plan for Phase 1.
