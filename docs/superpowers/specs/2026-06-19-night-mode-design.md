# Settings Dialog + Night Mode — Design Spec

**Date:** 2026-06-19
**Status:** Pending spec review
**Project root:** `F:/globe`

## 1. Overview

Move configuration toggles out of the crowded tray context menu into a dedicated
**Settings** dialog. The dialog controls the existing grid overlay and a new
**night mode** selector. The user can choose between a simple dynamic-shading
night side and a city-lights night texture.

The dialog and all user-facing strings support **English** and **Simplified
Chinese** via Qt's translation system.

## 2. Goals

- Keep the tray menu small and focused on view/navigation actions.
- Make grid and night-mode settings discoverable and easy to change.
- Provide two mutually exclusive night renders:
  - **Simple Night** — darkened day map only, no city-lights texture.
  - **Realistic Night** — NASA Black Marble city-lights texture on the night
    side, blended by the dynamic terminator.
- Support runtime language switching between English (`en`) and Simplified
  Chinese (`zh_CN`).
- Persist all settings to the existing `config.json`.

## 3. Non-Goals

- Per-pixel customization of grid color/density (kept at 15° warm amber).
- Advanced graphics settings (quality tier, MSAA, etc.).
- Auto-detection of system language on first launch.
- Full multi-language support beyond English and Simplified Chinese.

## 4. Decisions

| Topic | Decision |
|---|---|
| Settings entry | Single **Settings…** item in the tray menu |
| Grid toggle | Checkbox inside the dialog |
| Night mode | Radio group: **Simple Night** / **Realistic Night** |
| Night mode default | `simple` |
| Grid default | `false` |
| Language | Combo box: **English** / **简体中文** |
| Language default | `en` |
| i18n mechanism | Qt `tr()` + `.ts`/`.qm` files |
| Dialog style | Standard `QDialog` with a form layout |

## 5. Architecture

### 5.1 Config layer

`ConfigManager` gains:

- `QString nightMode() const` / `void setNightMode(const QString &)`
  - persisted as `"nightMode": "simple" | "texture"`
- `QString language() const` / `void setLanguage(const QString &)`
  - persisted as `"language": "en" | "zh_CN"`
- existing `bool showGrid() const` / `void setShowGrid(bool)` remains

Default values: `nightMode="simple"`, `language="en"`, `showGrid=false`.

### 5.2 Settings dialog

New class `SettingsDialog`:

- Modal `QDialog` with a form layout.
- Widgets:
  - **Language** label + `QComboBox` (`en`, `zh_CN`).
  - **Show Grid** `QCheckBox`.
  - **Night Mode** label + `QGroupBox` containing two `QRadioButton`s:
    - Simple Night
    - Realistic Night (city lights)
  - **OK** and **Cancel** buttons.
- On **OK**:
  - Writes all values to `ConfigManager`.
  - Emits a signal so `main.cpp` can reload the translator and notify the
    renderer.
  - Saves config.
- On **Cancel**: closes without writing.
- Strings are wrapped with `tr()` for translation.

### 5.3 Tray menu

`SystemTray`:

- Replaces the checkable **Show Grid** action with a single **Settings…**
  action that opens `SettingsDialog`.
- Keeps existing view actions: **Reset View**, **Center on Me**, etc.

### 5.4 Renderer layer

`GlobeRenderer`:

- Gains `void setUseNightTexture(bool)`.
- Forwards the state to `earth.frag` as `float uUseNightTexture`.

### 5.5 Shader layer (`earth.frag`)

- When `uUseNightTexture < 0.5`:
  - The sampled night texture is replaced by black/empty.
  - The dark side becomes a dim day map plus the existing faint airglow.
- When `uUseNightTexture > 0.5`:
  - Behaves as today: night texture blended by the dynamic terminator.
- The dynamic terminator (`dayFactor`) is always active; only the source of the
  night-side pixels changes.

### 5.6 Application wiring (`main.cpp`)

- Load `config.language()` at startup and install the matching `QTranslator`.
- Create `SettingsDialog` once, connect its accepted signal to:
  - `renderer.setShowGrid(...)`
  - `renderer.setUseNightTexture(...)`
  - Retranslate the tray menu and any visible UI.
- Initialize renderer state from config before the first frame.

## 6. Internationalization

- All user-visible strings in `SettingsDialog`, `SystemTray`, and `main.cpp`
  are wrapped with `tr()`.
- Translation files live under `i18n/`:
  - `globe_en.ts`
  - `globe_zh_CN.ts`
- CMake runs `qt_add_translations` (or the Qt 6 equivalent) to compile `.ts`
  into `.qm` and embed them as resources.
- At runtime `main.cpp` loads the `.qm` matching `config.language()`.

## 7. Testing

- **Config round-trip:** `ConfigManager` saves/reloads `showGrid`,
  `nightMode`, and `language`.
- **Renderer state:** `GlobeRenderer` forwards `useNightTexture` to the
  shader uniform.
- **Dialog state:** opening the dialog, changing values, clicking OK updates
  config; Cancel leaves config unchanged.
- **Visual smoke tests:**
  - Simple night mode: night side shows dim land/sea, no city lights.
  - Texture night mode: night side shows city lights.
  - Grid on/off works as before.
- **i18n smoke test:** switching to `zh_CN` shows Simplified Chinese labels.
- **Build/test:** existing Qt Test suite continues to pass.

## 8. Compatibility & Performance

- No extra draw calls or textures; the night texture is already loaded.
- The shader branch is a single uniform check.
- Qt 6 translation workflow is used; requires the Linguist tools available in
  the Qt installation.

## 9. Future Considerations (out of scope)

- More languages.
- More settings in the dialog (quality tier, cloud layer, rotation speed).
- Apply translations without restarting the dialog.
