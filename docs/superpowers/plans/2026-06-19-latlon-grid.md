# Optional Lat/Lon Grid Overlay — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a configurable 15° lat/lon grid overlay to the existing desktop globe, controlled by a new `showGrid` config key and drawn procedurally in the Earth fragment shader.

**Architecture:** A new `showGrid` boolean travels from `ConfigManager` → `main.cpp` → `GlobeRenderer` → `earth.frag` uniform `uShowGrid`. The shader computes per-fragment latitude/longitude, finds the nearest 15° parallel/meridian, anti-aliases a ~1 px warm-amber line, and blends it over the globe surface before the location beacon is drawn.

**Tech Stack:** Qt6/C++, CMake, raw OpenGL 3.3/GLSL, Qt Test.

---

## File Structure

- **Create:** none
- **Modify:**
  - `src/globe/ConfigManager.h` — add `showGrid` accessor/mutator and member
  - `src/globe/ConfigManager.cpp` — load/save the new key
  - `tests/test_configmanager.cpp` — verify persistence
  - `src/globe/render/GlobeRenderer.h` — add `setShowGrid(bool)`
  - `src/globe/render/GlobeRenderer.cpp` — forward uniform `uShowGrid`
  - `src/globe/render/shaders/earth.frag` — procedural grid block
  - `main.cpp` — wire config value into renderer at startup

---

## Task 1: ConfigManager `showGrid`

**Files:**
- Modify: `src/globe/ConfigManager.h:23` (insert accessors)
- Modify: `src/globe/ConfigManager.h:34` (insert member)
- Modify: `src/globe/ConfigManager.cpp:48` (load key)
- Modify: `src/globe/ConfigManager.cpp:57` (save key)
- Test: `tests/test_configmanager.cpp:11`

- [ ] **Step 1: Add accessor declarations in header**

  Insert after the existing `homeLongitude` line:

  ```cpp
  bool showGrid() const;        void setShowGrid(bool v);
  ```

- [ ] **Step 2: Add member variable**

  Insert after `m_locationOptIn`:

  ```cpp
  bool m_showGrid = false;
  ```

- [ ] **Step 3: Implement accessors in `ConfigManager.cpp`**

  Add after `setHomeLatitude`:

  ```cpp
  bool ConfigManager::showGrid() const { return m_showGrid; }
  void ConfigManager::setShowGrid(bool v) { m_showGrid = v; }
  ```

- [ ] **Step 4: Load the key in `load()`**

  Add after the `homeLongitude` load line:

  ```cpp
  m_showGrid = obj.value("showGrid").toBool(m_showGrid);
  ```

- [ ] **Step 5: Save the key in `save()`**

  Add after the `homeLongitude` save line:

  ```cpp
  o["showGrid"] = m_showGrid;
  ```

- [ ] **Step 6: Add a failing persistence test**

  In `tests/test_configmanager.cpp`, update `savesAndLoadsValues` to set/verify the new value:

  ```cpp
  void savesAndLoadsValues() {
      QTemporaryDir dir;
      ConfigManager c(dir.path());
      c.setWindowX(123); c.setWindowY(456);
      c.setDiameter(420); c.setQualityTier(ConfigManager::HD);
      c.setFpsCap(30); c.setLocationOptIn(true); c.setHomeLongitude(116.4);
      c.setShowGrid(true);
      c.save();

      ConfigManager c2(dir.path());
      QCOMPARE(c2.windowX(), 123);
      QCOMPARE(c2.windowY(), 456);
      QCOMPARE(c2.diameter(), 420);
      QCOMPARE(c2.qualityTier(), ConfigManager::HD);
      QCOMPARE(c2.fpsCap(), 30);
      QVERIFY(c2.locationOptIn());
      QCOMPARE(c2.homeLongitude(), 116.4);
      QVERIFY(c2.showGrid());
  }
  ```

- [ ] **Step 7: Run the test and confirm it fails**

  ```bash
  cmake --build build --target test_configmanager
  ctest --test-dir build -R test_configmanager --output-on-failure
  ```

  Expected: build error or test failure because `showGrid` is not yet defined.

- [ ] **Step 8: Build and verify the test passes**

  ```bash
  cmake --build build --target test_configmanager
  ctest --test-dir build -R test_configmanager --output-on-failure
  ```

  Expected: PASS.

- [ ] **Step 9: Commit**

  ```bash
  git add src/globe/ConfigManager.h src/globe/ConfigManager.cpp tests/test_configmanager.cpp
  git commit -m "feat(config): add showGrid option for lat/lon grid overlay"
  ```

---

## Task 2: Renderer uniform plumbing

**Files:**
- Modify: `src/globe/render/GlobeRenderer.h:22` (insert method)
- Modify: `src/globe/render/GlobeRenderer.h:42` (insert member)
- Modify: `src/globe/render/GlobeRenderer.cpp:51` (implement setter)
- Modify: `src/globe/render/GlobeRenderer.cpp:228` (set uniform)

- [ ] **Step 1: Declare `setShowGrid` in the header**

  Insert after `setHomeLocation`:

  ```cpp
  void setShowGrid(bool v);
  ```

- [ ] **Step 2: Add member variable**

  Insert after `m_hasHome`:

  ```cpp
  bool m_showGrid = false;
  ```

- [ ] **Step 3: Implement the setter**

  Add after `setHomeLocation`:

  ```cpp
  void GlobeRenderer::setShowGrid(bool v) { m_showGrid = v; }
  ```

- [ ] **Step 4: Set the uniform in `render()`**

  After the existing `uHasSpecular` uniform (around line 228), add:

  ```cpp
  m_earthProg->setUniformValue("uShowGrid", m_showGrid ? 1.0f : 0.0f);
  ```

- [ ] **Step 5: Build to ensure no compile errors**

  ```bash
  cmake --build build --target globe_lib
  ```

  Expected: builds successfully.

- [ ] **Step 6: Commit**

  ```bash
  git add src/globe/render/GlobeRenderer.h src/globe/render/GlobeRenderer.cpp
  git commit -m "feat(renderer): forward showGrid state to earth.frag uniform"
  ```

---

## Task 3: Procedural grid in `earth.frag`

**Files:**
- Modify: `src/globe/render/shaders/earth.frag:29` (add uniform)
- Modify: `src/globe/render/shaders/earth.frag:99` (insert grid block before the beacon)

- [ ] **Step 1: Add the uniform declaration**

  Insert after `uTime`:

  ```glsl
  uniform float uShowGrid;
  ```

- [ ] **Step 2: Insert the grid function before `main()`**

  After the existing `sphereToUV` function, add:

  ```glsl
  const float kGridStepRad = 3.14159265 / 12.0;        // 15 degrees
  const float kGridLinePx  = 1.2;                      // ~1 px anti-aliased line
  const vec3  kGridColor   = vec3(1.0, 0.70, 0.28);    // warm amber
  const float kGridAlpha   = 0.22;

  float gridLine(float coord) {
      float d = abs(fract(coord / kGridStepRad + 0.5) - 0.5) * kGridStepRad;
      float w = fwidth(coord) * kGridLinePx;
      return 1.0 - smoothstep(0.0, w, d);
  }
  ```

- [ ] **Step 3: Compute and blend the grid inside `main()`**

  Insert the following block right after the existing specular/glint block (before the `if (uHasHome > 0.5)` beacon block):

  ```glsl
  if (uShowGrid > 0.5) {
      float lat = asin(clamp(nObj.z, -1.0, 1.0));
      float lon = atan(nObj.y, nObj.x);
      float grid = max(gridLine(lat), gridLine(lon));

      // Fade lines toward the limb so they don't bunch/hard-edge at the disc edge.
      float viewDot = dot(nGeo, normalize(uViewPos - vWorld));
      grid *= smoothstep(0.0, 0.15, viewDot);

      color = mix(color, kGridColor, grid * kGridAlpha);
  }
  ```

- [ ] **Step 4: Build to catch shader/compile errors**

  ```bash
  cmake --build build --target globe
  ```

  Expected: builds successfully.

- [ ] **Step 5: Commit**

  ```bash
  git add src/globe/render/shaders/earth.frag
  git commit -m "feat(shader): procedural 15° lat/lon grid overlay"
  ```

---

## Task 4: Wire config to renderer at startup

**Files:**
- Modify: `main.cpp:43` (insert setShowGrid call)

- [ ] **Step 1: Initialize `showGrid` from config**

  After the existing `setHomeLocation` line, add:

  ```cpp
  w.view()->renderer().setShowGrid(config.showGrid());
  ```

- [ ] **Step 2: Build the app**

  ```bash
  cmake --build build --target globe
  ```

  Expected: builds successfully.

- [ ] **Step 3: Commit**

  ```bash
  git add main.cpp
  git commit -m "feat(app): wire showGrid config to renderer at startup"
  ```

---

## Task 5: Verification

- [ ] **Step 1: Run the full test suite**

  ```bash
  ctest --test-dir build --output-on-failure
  ```

  Expected: all tests pass.

- [ ] **Step 2: Visual smoke test — grid enabled**

  Edit `%LOCALAPPDATA%\Globe\config.json` (or the dev config path) to add:

  ```json
  "showGrid": true
  ```

  Run:

  ```bash
  .\build\globe.exe
  ```

  Expected: faint warm-amber lat/lon grid lines appear over the globe and rotate with it.

- [ ] **Step 3: Visual smoke test — grid disabled/default**

  Remove or set `"showGrid": false`, then restart.

  Expected: globe renders exactly as before with no grid lines.

- [ ] **Step 4: Commit verification notes (optional)**

  If any tweaks were needed, commit them; otherwise no extra commit.

---

## Spec Coverage Check

| Spec Requirement | Plan Task |
|---|---|
| Config key `showGrid` (default `false`) | Task 1 |
| Renderer forwards `uShowGrid` | Task 2 |
| Shader draws 15° amber anti-aliased grid | Task 3 |
| Startup wiring from config | Task 4 |
| Config round-trip test | Task 1 |
| Visual smoke test | Task 5 |
| No tray toggle / labels / multiple densities | Non-goals respected; no code added |

## Placeholder / Consistency Check

- No `TBD`, `TODO`, or vague steps remain.
- Method signatures match across `ConfigManager.h/.cpp`, `GlobeRenderer.h/.cpp`, and `main.cpp`.
- Uniform name `uShowGrid` is identical in C++ and GLSL.
- All paths are absolute to the project root.
