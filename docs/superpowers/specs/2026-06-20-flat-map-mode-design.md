# Flat Map Mode — Design Spec

**Date:** 2026-06-20
**Status:** Draft — awaiting user review, then implementation plan
**Decision owner:** User approved Approach A (new flat-map GL shader + quad, toggled in the existing widget).

## Goal

Add a **"Flat Map" mode** to the Earth widget: a toggle that reshapes the circular 3D globe into a **flat rectangular (equirectangular) map of the whole world**, rendering the existing day texture flat with a **live, real-time day/night terminator**. The flat map shows the **true current sun position** (no toy spin), so it is a more faithful real-world day/night simulation than the spinning globe.

Reference: mission-control / rocket-telemetry style "unwound globe" maps — a 2:1 rectangle filling the frame, day side lit, night side dark, soft terminator down the middle.

## Motivation

The spinning globe "paints" day/night onto the surface and spins fast (~2880×) for toy appeal; its terminator is decorative. The flat map fixes the whole world in place (ECEF), so the terminator is driven by the **real wall-clock sun** from `SunModel`, drifting ~15°/hour exactly as Earth does. This makes the flat map the **accurate real-world view**, complementing the playful globe.

## Non-Goals

- Pan / zoom around the map (whole-world fixed view only).
- Text labels (countries/oceans) on the map.
- Rocket trajectory / vehicle tracking overlays.
- Replacing the globe — the globe remains the default; flat map is an opt-in toggle.
- 3D relief / ocean glint on the flat map (it's a flat 2D map; those are globe-only).

## Architecture (Approach A)

A new flat-map GL render path in the existing `celestial` library, toggled at the widget level. No new executable, no new widget class.

```
src/celestial/
├── CelestialBody.{h,cpp}     (+ renderMap() method, m_mapProg, quad geometry)
├── CelestialWidget.{h,cpp}   (+ ViewMode flag, mask/resize on toggle)
├── ConfigManager.{h,cpp}     (+ viewMode setting)
├── SystemTray.{h,cpp}        (+ checkable "Flat Map" action)
└── shaders/
    ├── map.vert              (NEW — fullscreen quad)
    └── map.frag              (NEW — flat day/night + grid + marker)
src/apps/earth/main.cpp        (wire tray toggle → widget mode + config)
tests/
└── test_flatmap_terminator.cpp (NEW — GL-free day/night helper)
```

## Components

### 1. `map.vert` / `map.frag`

A fullscreen quad (clip-space triangle pair). UV ∈ [0,1]² maps **directly** to longitude/latitude (the day/night textures are already equirectangular 2:1, so UV *is* lon/lat — no sphere parametrization needed).

`map.frag` per fragment:
- `lon = (uv.x * 2 − 1) * π`, `lat = (uv.y * 2 − 1) * π/2`.
- ECEF surface normal at that point: `p = vec3(cos(lat)*cos(lon), cos(lat)*sin(lon), sin(lat))`.
- `sun = normalize(uSunDir)` — the **real** ECEF sun direction from `SunModel`.
- `cosGeo = dot(p, sun)`; `dayFactor = smoothstep(−0.10, 0.20, cosGeo)` — the **same soft terminator width** as the globe, so the two views agree.
- Day color: `texture(uDay, uv)` (mipmaps OK here — the flat quad has no interior UV discontinuity, so **no date-line seam**, simpler than the sphere path).
- Night side per **Night Mode**: `uUseNightTexture == 1` → city-lights `uNight` texture; else dimmed day. `darkSide = night*0.8 + day*0.35 + airglow`; `color = mix(darkSide, day, dayFactor)`.
- **No brown dusk tint** (deliberately omitted — the user dislikes it on the globe; the map terminator is a clean day/night transition, matching the reference image).
- **Grid:** straight vertical lines every 15° lon, horizontal every 15° lat, `fwidth`-AA, warm amber, limb-fade N/A (flat). Reuses the globe's grid constants.
- **Home marker:** the "you are here" dot plotted at `(homeLat, homeLon) → uv`, simple 2D exponential falloff (no angular 3D distance needed). Same look as the globe beacon.

### 2. `CelestialBody` — `renderMap()`

A new public method paralleling `render()`:
- `initialize()` additionally compiles `m_mapProg = makeProgram("map.vert","map.frag")` and builds a fullscreen-quad VAO (`buildQuad()`).
- `renderMap()` binds `m_mapProg`, sets uniforms (`uDay`, `uNight`, `uSunDir`, `uUseNightTexture`, `uShowGrid`, `uHomeLat`/`uHomeLon`, `uHasHome`, `uTime`, `uHasDay`, `uHasNight`), draws the quad.
- `render()` (the 3D globe path) is unchanged.

`uSunDir` is the raw ECEF sun (`m_sun->sunDirection()`), NOT the spun `sunWorld` — the map is fixed ECEF, so it uses the true sun.

### 3. `CelestialWidget` — mode toggle

- New `enum class ViewMode { Globe, FlatMap };` and `ViewMode m_viewMode = Globe;`.
- `void setViewMode(ViewMode m)`:
  - `FlatMap`: `clearMask()` (rectangle), resize to **2:1 keeping the width** (height = width / 2), set `m_viewMode`.
  - `Globe`: restore the elliptical mask (in `resizeEvent`), resize back to **square** (height = width), set `m_viewMode`.
- `paintGL()` branches: `Globe → m_body.render();` / `FlatMap → m_body.renderMap();`.
- `resizeEvent`: only applies the elliptical mask in `Globe` mode (in `FlatMap` the widget stays rectangular).
- `wheelEvent`: in `FlatMap`, resize preserving 2:1 (adjust width, set height = width/2). In `Globe`, unchanged (square).
- Mouse: move gesture (Alt/middle-drag) still moves the widget in both modes. Left-drag camera-rotate is a **no-op** in `FlatMap` (no camera on a flat map).

### 4. `ConfigManager` — `viewMode`

New persisted field: `QString viewMode() const;` / `setViewMode(const QString&)` accepting `"globe"` (default) or `"map"`. JSON key `"viewMode"`. Backward-compatible default = `"globe"`.

### 5. `SystemTray` — toggle entry

A checkable **"Flat Map"** action in the tray menu (alongside Show Grid, Settings, etc.). Toggling it emits a signal `flatMapToggled(bool)` that `main.cpp` wires to `widget.setViewMode(...)` + `config.setViewMode(...)` + `config.save()`.

### 6. `main.cpp` wiring

On startup: read `config.viewMode()` → `widget.setViewMode(...)`. Connect `SystemTray::flatMapToggled` → apply + persist. The existing `TimeController` already drives repaints and refreshes the sun every minute, so the terminator drifts live in flat-map mode with no extra wiring.

## Data Flow

`SunModel` (real UTC, refreshed every 60 s by `TimeController`) → `sunDirection()` (ECEF unit) → `CelestialBody::renderMap()` sets `uSunDir` → `map.frag` computes per-pixel `dot(p, sun)` → soft `dayFactor` → lit day texture vs. dark night side. No spin, no model rotation — the map is the fixed ECEF world. Result: the terminator is the **actual** current one, drifting ~15°/hour.

## Design Decisions (resolved)

| Question | Decision |
|----------|----------|
| How does flat map relate to the globe? | Toggle on the existing widget |
| Shape / view | Whole world, 2:1 rectangle |
| Night side / terminator | Reuse current Night Mode; soft gradient terminator |
| Motion / features | Live real-time terminator; grid + marker carry over |
| Brown dusk tint on terminator | **Off** (clean transition) |
| 2:1 resize anchor | Keep the **width**, height = width/2 |
| Spin | None — fixed ECEF map (real-world accurate) |

## Edge Cases / Compatibility

- **Fullscreen auto-hide:** applies in both modes (the widget hides regardless of shape).
- **Singleton:** unchanged.
- **Persistence:** `viewMode` survives restart; a user who toggled to map mode reopens in map mode.
- **Quality tier / night mode / language:** all respected by the map shader via the same uniforms/config.
- **Drag in flat map:** left-drag (camera) is a no-op; move gesture still works; double-click reset is a no-op in flat map (nothing to reset — could later center the map, out of scope).
- **Window position:** preserved across the toggle (only size + mask change).

## Testing

- **Unit:** `ConfigManager.viewMode` round-trip (globe ↔ map, default globe).
- **Unit (GL-free):** a static helper `float TerminatorModel::dayFactor(QVector3D sunDir, double latDeg, double lonDeg)` returning the `smoothstep` value, shared by the shader's math. Test: sub-solar point → day (≈1); antipode → night (≈0); equator at dawn longitude → mid (≈0.5). This pins the terminator math independent of GL.
- **Visual smoke:** toggle on → 2:1 rectangle, live terminator, grid + marker visible; toggle off → circular spinning globe. Manually verified.

## Open Questions

None — all resolved during brainstorming.

## Next Step

After this spec is approved by the user, invoke the `writing-plans` skill to produce the implementation plan.
