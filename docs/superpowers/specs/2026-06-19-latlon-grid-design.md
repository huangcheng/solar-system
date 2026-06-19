# Optional Lat/Lon Grid Overlay — Design Spec

**Date:** 2026-06-19
**Status:** Pending spec review
**Project root:** `F:/globe`

## 1. Overview

Add an optional geographic grid overlay (parallels and meridians every 15°) to
the existing desktop Earth globe. The grid is drawn procedurally inside the
Earth fragment shader, is **off by default**, and is enabled manually through
`config.json`.

## 2. Goals

- Provide a faint, non-obstructive lat/lon reference on demand.
- Keep the existing photorealistic look unchanged when the grid is disabled.
- Make the grid feel like part of the globe surface (rotates with the Earth,
  not a fixed screen overlay).
- Avoid extra geometry, draw calls, or textures.

## 3. Non-Goals

- Degree labels or coordinate read-outs.
- Multiple grid densities or colors (fixed 15° spacing, warm amber).
- Celestial / sky grid.

## 4. Decisions (from brainstorming)

| Topic | Decision |
|---|---|
| Visual style | Faint reference lines |
| Default state | Off |
| Toggle mechanism | `config.json` key **plus** a checkable tray menu item |
| Spacing | 15° parallels and meridians |
| Color | Warm amber (`#ffb347`) |
| Labels | None |
| Line quality | Anti-aliased, ~1 px wide |
| Implementation | Procedural fragment shader (Approach 1) |

## 5. Architecture

### 5.1 Config layer

- `ConfigManager` gains `bool showGrid() const` and `void setShowGrid(bool)`.
- The value is persisted in `config.json` as `"showGrid": true/false`.
- Default is `false`.

### 5.2 Renderer layer

- `GlobeRenderer` gains `void setShowGrid(bool)`.
- The state is forwarded to the Earth fragment shader as uniform `float uShowGrid`.
- No new VAOs, draw calls, or textures are introduced.

### 5.3 Shader layer (`earth.frag`)

The grid is computed from the existing per-fragment latitude and longitude.

- Compute the angular distance from the current fragment to the nearest 15°
  parallel and to the nearest 15° meridian.
- Convert each distance to an anti-aliased line intensity using `smoothstep`
  (and optionally `fwidth`) so the line stays ~1 px wide at all viewport sizes.
- Line color: warm amber, low opacity (around 0.22), blended over the existing
  day/night/atmosphere result.
- Fade the grid near the limb to avoid hard edge artifacts.
- When `uShowGrid < 0.5`, the entire grid branch is skipped.

### 5.4 Tray menu

- `SystemTray` exposes a `toggleShowGrid(bool)` signal and a checkable
  **Show Grid** action.
- `main.cpp` initializes the action's checked state from `config.showGrid()` and
  connects the signal to update both `ConfigManager` and `GlobeRenderer`, then
  saves the config.

### 5.5 Application startup

- `main.cpp` reads `config.showGrid()` after loading config and calls
  `renderer.setShowGrid(...)` before the first frame.

## 6. Rendering Details

- The grid uses the same tangent-space/analytic-sphere UV math that already
  fixes the date-line seam, so meridian lines wrap continuously.
- Because the grid is evaluated in world/object space, it rotates naturally
  with the Earth model matrix.
- The anti-aliased falloff prevents the lines from aliasing at grazing angles
  and at the limb.

## 7. Testing

- **Config round-trip:** `ConfigManager` saves and reloads `showGrid` correctly.
- **Visual smoke test:** launch the app with `"showGrid": true`; amber lines
  are visible and rotate with the globe.
- **Default behavior:** with the key absent or `false`, the globe renders
  identically to before.
- **Build/test:** existing Qt Test suite continues to pass.

## 8. Compatibility & Performance

- No performance impact when disabled (uniform branch + compile-time constants).
- When enabled, cost is a handful of additional fragment-shader instructions.
- Works on the existing OpenGL 3.3 context and all supported platforms.

## 9. Future Considerations (out of scope)

- Tray menu toggle for quick on/off.
- Configurable grid density (e.g., 10°, 30°).
- Coordinate labels at the equator / prime meridian.
- Separate color or opacity controls.
