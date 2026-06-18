# Globe Desktop Widget — Design Spec

**Date:** 2026-06-18
**Status:** Approved (pending spec review)
**Project root:** `F:/globe`

## 1. Overview

A native, cross-platform desktop widget that renders a photorealistic, real-time
Earth globe. The globe orbits like the real Earth, reflects the current day/night
terminator computed from the system clock, and lives on the desktop as a
frameless, always-on-top, circular widget (inspired by the Seelie desktop pet's
window shell at `F:/seelie`).

The widget is built on the existing empty Qt6/C++ project at `F:/globe` and uses
raw OpenGL/GLSL for high-quality rendering.

## 2. Goals

- Photorealistic Earth with **HD NASA satellite imagery** (Blue Marble day map,
  Black Marble night-lights map), **real-time day/night terminator**, **moving
  cloud layer**, and **atmosphere limb glow**.
- **Sun-centric default view**: the sunlit hemisphere faces the user; the globe's
  surface drifts at Earth's real rate (~15°/hour) under the sun. This is the
  "reflect to real-time daylight" source of truth and is computed fully offline
  from the current UTC.
- Frameless, always-on-top, circular, **resizable** desktop widget with system
  tray, context menu, and persisted position/size.
- Draggable/orbitable/zoomable, with a reset that re-locks to the real-time
  sun-centric view.
- Optional **user location** features: home pin, "center on me" view, and
  local solar info in the tray tooltip (permission-gated, opt-in, local-only).
- Small footprint: native Qt binary plus bundled textures, far smaller and
  lighter than an Electron app.

## 3. Non-Goals (v1)

- Satellite/orbit labels and spacecraft overlays (seen in the reference image
  but explicitly out of scope).
- Live fetched imagery / cloud updates from the network. All textures are
  bundled and offline.
- Starfield background (window is exactly the circular globe; no surrounding
  background).
- Mobile platforms.
- Full physically-based atmospheric raymarching (see Architecture Option C,
  rejected).

## 4. Decisions (from brainstorming)

| Topic | Decision |
|---|---|
| Platform / stack | Native Qt6/C++, cross-platform (Windows, macOS, Linux) |
| Rendering backend | Raw OpenGL/GLSL inside Qt6 (not Qt3D / not an external engine) |
| Visual style | Photorealistic, as real as possible |
| Render architecture | **Option B — layered multi-pass** (Earth + clouds + atmosphere) |
| Rotation / orientation | Real-time **sun-centric** by default; drag/zoom + reset |
| Shape | Circular, frameless, **resizable** (wheel = resize) |
| Visual layers | Day map, night city lights, terminator, atmosphere glow, clouds |
| Textures | **Bundled HD, offline**; quality tiers + auto-downscale |
| Desktop behavior | Seelie-style: frameless, always-on-top, draggable, tray, context menu, persistence |
| Location | Optional, permission-gated; home pin + "center on me" + tray solar info |

## 5. Architecture & Components

### 5.1 Shell (mirrors Seelie's proven pattern)

- `QApplication` + `GlobeWindow` — a `QWidget`, frameless / translucent /
  always-on-top / `Qt::Tool`. On Windows, reuses the DWM frameless-attribute
  routine from Seelie's `PlatformWindow` (no-ops on other platforms).
- `SystemTray` + right-click context menu: **Show/Hide**, **Reset View**,
  **Center on me**, **Settings**, **About**, **Quit**.
- `ConfigManager` (JSON) — persists window position, diameter, quality tier,
  FPS cap, cloud speed, "home longitude", and the location opt-in flag.

### 5.2 Rendering core

- `GlobeView` (`QOpenGLWidget`) — owns the GL context and drives `paintGL`
  each frame.
- `GlobeRenderer` — manages sphere VAOs, the three shader programs, and the
  textures. Per frame it draws, in order:
  1. **Earth sphere** — day map + night-lights map + computed terminator from
     `sunDir`.
  2. **Cloud sphere** — slightly larger, semi-transparent, slow independent
     rotation.
  3. **Atmosphere shell** — slightly larger back-face sphere, Fresnel +
     scattering glow, additive blend.
- `SunModel` — computes the **sub-solar latitude/longitude** and `sunDir`
  from the current UTC via a standard solar-position algorithm (includes
  Earth's 23.44° axial tilt and seasonal declination). Pure, offline, unit-testable.
- `CameraController` — sun-centric default; drag = yaw/pitch offset, wheel =
  zoom/resize, double-click / Reset = clear offset and re-lock to the real-time
  sun view.
- `TimeController` — `QTimer`; recomputes `sunDir` and advances Earth's rotation
  angle so continents drift under the sun at the real rate.
- `AssetManager` — loads bundled HD day / night-lights / cloud textures into GL
  textures, with optional auto-downscale on low VRAM.

### 5.3 Location (optional)

- `LocationProvider` — single interface wrapping the platform geolocation API:
  - Windows: WinRT `Windows.Devices.Geolocation`.
  - macOS: CoreLocation `CLLocationManager`.
  - Linux: GeoClue2 (with a GeoIP fallback).
- Permission-gated, opt-in, and **never transmitted anywhere** — coordinates
  stay local.
- Drives three features (Section 7).

### 5.4 Data flow

`system clock (UTC)` → `SunModel.sunDir(UTC)` → shader uniform → drives Earth
day/night lighting + cloud lighting + atmosphere limb. Earth's model matrix is
set so the sub-solar longitude sits at screen center by default; user drag adds
a yaw/pitch offset on top; Reset clears it.

## 6. Rendering & Shader Model

**Geometry & frames**
- UV sphere, high subdivision (e.g. 128×256), best fit for equirectangular
  Blue Marble textures.
- Earth-fixed frame; `uv.x = (lon+180)/360`, `uv.y = (lat+90)/180`.
- `SunModel` returns sub-solar latitude/longitude at the current UTC (axial
  tilt + seasonal declination) → `sunDir` in the same frame.

**Sun-centric default**
- Earth's model matrix rotates so the sub-solar longitude is at screen center;
  continents drift at ~15°/hour as UTC advances. Axial tilt applied for correct
  seasons. User drag adds a yaw/pitch offset; Reset clears it.

**Layer 1 — Earth fragment shader**
- Sample HD day map + night-lights map.
- `cosSun = dot(normal, sunDir)`; terminator = `smoothstep(-0.1, 0.2, cosSun)`
  → soft day/night blend.
- Night side shows city lights (suppressed on day side); optional subtle ocean
  sun-glint; optional warm twilight tint at the terminator.

**Layer 2 — Cloud sphere**
- Slightly larger sphere, semi-transparent cloud alpha map, lit by the same
  `sunDir`, rotating at its own slow offset speed.

**Layer 3 — Atmosphere shell**
- Back-face sphere, slightly larger; Fresnel limb glow + simple Rayleigh-style
  scattering approximation, day/night-aware (brighter on the sunlit limb),
  additive blend.

**Silhouette & widget shape**
- MSAA for smooth edges; the fragment shader discards outside the disc radius
  so the frameless window renders as a perfect circle on the transparent
  desktop surface.

**Performance**
- Target 60 fps when visible; configurable FPS cap (default 60, 30 on battery);
  render throttled/paused when hidden.

## 7. Location Features

Driven by `LocationProvider` (opt-in):

- **Home marker** — a small pin rendered on the globe at the user's coordinates.
- **"Center on me"** — an alternative view (button/menu) that locks the camera
  over the user's longitude, alongside the sun-centric default.
- **Tray/tooltip info** — local day/night status, sunrise/sunset, and local
  solar time, refreshed periodically.

## 8. Interaction & Window Model

**Globe controls**
- **Left-drag** = orbit (yaw/pitch offset on top of the sun-centric base).
- **Mouse wheel** = zoom, which **resizes the circular widget** (the wheel is
  the resize mechanism, since a frameless circle has no edge handles). Clamped
  to a min/max diameter.
- **Double-click / "Reset View" menu** = clear offset, re-lock to real-time
  sun-centric view.

**Moving the widget**
- **Alt+Left-drag or Middle-drag** = move the widget on the desktop.
- Context menu also has **"Lock position"** and **"Center on me"**.

**Window shell**
- Frameless, translucent, always-on-top, `Qt::Tool`, no taskbar entry,
  `WA_ShowWithoutActivating` + `WA_TranslucentBackground` + `WA_NoSystemBackground`.
- **Circular click mask** (`QRegion`) sized to the disc so the transparent
  corners are click-through; recomputed on every resize.
- Windows DWM frameless attributes via Seelie's `PlatformWindow` helper.
- **Persistence & DPI:** window position + diameter saved to config and
  restored on launch (clamped to the current screen); honors `devicePixelRatio`
  so HD textures stay crisp on HiDPI.

## 9. Assets & Build

**Bundled HD textures (offline, public-domain NASA imagery)**
- Day map — Blue Marble true-color equirectangular.
- Night-lights map — Black Marble (VIIRS) equirectangular.
- Clouds — semi-transparent cloud layer (PNG with alpha), rotates slowly.
- Atmosphere — computed in-shader, no texture.
- Attribution included in the About dialog (NASA public domain).

**Quality vs. memory**
- Three RGBA textures at 8K can consume ~400 MB of VRAM — too much for a
  desktop widget. Ship at **8K (HD) / 4K (Medium) / 2K (Low)** tiers; a
  **Quality** setting selects the tier, with **auto-downscale** when free VRAM
  is low.
- Textures ship as **files next to the binary** (not embedded in the `.qrc`)
  so the executable stays lean and users can later swap custom maps via a
  config path.

**Build system**
- Extend the existing `F:/globe` CMake; add Qt modules: `OpenGL`,
  `OpenGLWidgets`, `Gui`, `Widgets` (and `Network` only if a future optional
  cloud-fetch is added; not needed for v1).
- Source layout mirrors Seelie's clean separation, e.g.
  `src/globe/{globeview,globerenderer,sunmodel,cameracontroller,locationprovider,configmanager,...}.cpp/.h`.
- C++17, cross-platform (Windows / macOS / Linux). Deployment later:
  - **Windows:** `windeployqt` to gather Qt DLLs, then an **Inno Setup**
    installer (chosen over Qt IFW — easier to author and maintain).
  - **macOS:** `macdeployqt` → `.app` bundle / DMG.
  - **Linux:** AppImage tooling.

## 10. Testing & Error Handling

**Testing (Qt Test, like Seelie)**
- `SunModel` — deterministic checks: equinox noon sun due south, solstice
  declination ±23.44°, known sunrise/sunset times for a reference city,
  sub-solar longitude advances ~15°/hour.
- `LocationProvider` — mock geolocation; verify the permission-denied path
  disables location features without breaking the globe.
- `ConfigManager` — save/load, position clamped to screen, quality tier
  persistence.
- `CameraController` — orbit offset + Reset returns to sun-centric base.
- Rendering — manual visual check (pixels are hard to assert); ensure GL
  context creation is crash-safe and the widget degrades gracefully on
  unsupported hardware.

**Error handling / degradation**
- **No/old OpenGL** — detect context version; fall back to a lower tier, and
  ultimately to a static globe image instead of crashing.
- **Missing/corrupt texture** — drop to a lower tier or bundled fallback.
- **Location permission denied** — home pin / "center on me" / solar tooltip
  quietly disabled; sun-centric view unaffected.
- **Low VRAM** — auto-downscale tier; if still pressured, drop clouds then
  atmosphere.
- **Display sleep / DWM restart (Windows)** — reuse Seelie's `PlatformWindow`
  refresh routine to restore transparency and frameless attributes.
- **Multi-monitor / DPI change** — recompute circular mask and GL viewport on
  screen/layout changes.

## 11. Future Considerations (out of scope for v1)

- Optional live cloud imagery fetch (would re-enable the `Network` module).
- Custom texture packs / theme switching.
- Installer packaging (Inno Setup on Windows, DMG on macOS, AppImage on Linux).
- Specular ocean glint and physically-based atmosphere as a later quality pass.
