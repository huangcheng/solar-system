# Phase 5 — Polish Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: superpowers:subagent-driven-development or superpowers:executing-plans. Steps use checkbox (`- [ ]`) syntax.

**Goal:** Take the Phase 2–4 features from functional to polished: persistent labels, smooth orbit trails, Milky-Way/label/orbit toggles persisted to config, per-body advanced settings, full zh_CN translation, and performance/LOD tuning so the Solar System Mode stays smooth.

**Architecture:** No new subsystems — refines what Phases 2–4 built. Adds a `solar` config (`solar-config.json`) for scene preferences, a label render pass using `QPainter` over the GL widget, and a LOD cap for outer-planet textures.

**Tech Stack:** Qt6, OpenGL 3.3 Core, QPainter overlay.

---

## Task 1: Body labels in Solar System Mode

**Files:**
- Modify: `src/apps/solar/SolarSystemWindow.cpp`
- Modify: `src/celestial/SolarSystemScene.h/.cpp`

- [ ] **Step 1:** Add `scene->projectBodyScreenPositions(view, proj, viewport)` returning a `QVector<QPair<QString, QPointF>>` (name + screen pos) for each body within frustum.

- [ ] **Step 2:** In `SolarSystemWindow::paintGL`, after `scene.render`, enable `QPainter` on the GL widget and draw each name centered above its projected point. Respect `showLabels` toggle.

- [ ] **Step 3:** Build, verify labels track bodies as camera moves.

- [ ] Commit: `git commit -m "feat(solar): body labels overlay"`

---

## Task 2: Smooth orbit trails + toggles persisted

**Files:**
- Modify: `src/celestial/SolarSystemScene.cpp`
- Create: `src/celestial/SolarConfig.h/.cpp` (settings for solar.exe scene)

- [ ] **Step 1:** `SolarConfig` persists `showOrbits`, `showLabels`, `showMilkyWay`, `timeScale` to `AppConfigLocation/solar/config.json` (reuse ConfigManager pattern or a tiny dedicated struct).

- [ ] **Step 2:** Orbit trails: increase segments to 256, add per-vertex alpha fade so the trail is brighter near the planet. Add a `showOrbits` guard.

- [ ] **Step 3:** Bind the three toggles + slider to `SolarConfig` so they persist across runs.

- [ ] Commit: `git commit -m "feat(solar): smoother orbit trails and persisted scene preferences"`

---

## Task 3: Per-body advanced settings

**Files:**
- Modify: `src/celestial/SettingsDialog.h/.cpp`
- Modify: `src/apps/*/main.cpp` (via `BodyApp`)

- [ ] **Step 1:** Make `SettingsDialog` body-aware: it reads `BodyConfig` and adds body-specific controls:
  - Sun: corona intensity slider
  - Saturn: ring opacity + ring tilt
  - Jupiter: storm visibility (future)
  - All: spin-speed multiplier (already in Earth), visual size

- [ ] **Step 2:** Persist new keys via `ConfigManager` (extend its JSON schema; backward compatible).

- [ ] Commit: `git commit -m "feat: per-body advanced settings (Sun corona, Saturn rings)"`

---

## Task 4: Translation pass

**Files:**
- Modify: `globe_zh_CN.ts` (or rename to `celestial_zh_CN.ts`)
- Run `lupdate` / `lrelease`

- [ ] **Step 1:** Run `lupdate` to extract all new `tr()` strings from Phases 2–5.

- [ ] **Step 2:** Provide zh_CN translations for every new string (launcher labels, HUD, settings).

- [ ] **Step 3:** Verify language toggle in each app shows Chinese.

- [ ] Commit: `git commit -m "i18n: zh_CN translations for solar system UI"`

---

## Task 5: Performance / LOD

**Files:**
- Modify: `src/celestial/SolarSystemScene.cpp`
- Modify: `src/celestial/AssetManager.cpp`

- [ ] **Step 1:** Texture LOD: in Solar System Mode, cap planet textures at 2K (planets are small on screen); only Earth/Sun at full res when focused.

- [ ] **Step 2:** Frustum cull: skip bodies outside the camera frustum.

- [ ] **Step 3:** Profile with a 10-planet scene at 1080p; target steady 60 fps.

- [ ] Commit + tag: `git commit -m "perf: LOD + frustum culling for Solar System Mode" && git tag phase5-polish`

---

## Spec Coverage

| Spec item | Task |
|-----------|------|
| Labels | Task 1 |
| Orbit trails + toggles | Task 2 |
| Per-body settings | Task 3 |
| Translation | Task 4 |
| Performance/LOD | Task 5 |
