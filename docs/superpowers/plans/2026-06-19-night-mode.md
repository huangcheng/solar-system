# Settings Dialog + Night Mode Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a `SettingsDialog` for grid and night-mode options, split the two night rendering systems into mutually exclusive modes, and translate the UI into English and Simplified Chinese.

**Architecture:** A new modal `SettingsDialog` is opened from the tray menu. It writes `showGrid`, `nightMode`, and `language` to `ConfigManager`, which persists them to `config.json`. `GlobeRenderer` forwards the night-mode choice to `earth.frag` via `uUseNightTexture`. Qt `tr()` and a `globe_zh_CN.ts` file provide i18n.

**Tech Stack:** Qt 6 / C++17 / CMake / OpenGL 3.3 / Qt Linguist

---

## File Structure

| File | Responsibility |
|---|---|
| `src/globe/ConfigManager.h` / `.cpp` | Persist `showGrid`, `nightMode`, `language` |
| `src/globe/SettingsDialog.h` / `.cpp` | Modal dialog for settings |
| `src/globe/SystemTray.h` / `.cpp` | Tray menu: replace Show Grid with Settings… |
| `src/globe/render/GlobeRenderer.h` / `.cpp` | Forward `useNightTexture` to shader |
| `src/globe/render/shaders/earth.frag` | Use `uUseNightTexture` to pick night source |
| `main.cpp` | Install translator, wire dialog/renderer/config |
| `globe_zh_CN.ts` | Simplified Chinese translations |
| `CMakeLists.txt` | Add `SettingsDialog` sources to `globe_lib` |
| `tests/test_configmanager.cpp` | Verify new config round-trips |

---

### Task 1: Extend ConfigManager for night mode and language

**Files:**
- Modify: `src/globe/ConfigManager.h`
- Modify: `src/globe/ConfigManager.cpp`
- Test: `tests/test_configmanager.cpp`

- [ ] **Step 1: Add members and accessors in `ConfigManager.h`**

Add after the `showGrid` accessors:

```cpp
    QString nightMode() const;    void setNightMode(const QString &v);
    QString language() const;     void setLanguage(const QString &v);
```

Add private members after `m_showGrid`:

```cpp
    QString m_nightMode = QStringLiteral("simple");
    QString m_language  = QStringLiteral("en");
```

- [ ] **Step 2: Implement accessors in `ConfigManager.cpp`**

After the `showGrid` accessors, add:

```cpp
QString ConfigManager::nightMode() const { return m_nightMode; }
void ConfigManager::setNightMode(const QString &v) { m_nightMode = (v == QStringLiteral("texture") ? v : QStringLiteral("simple")); }
QString ConfigManager::language() const { return m_language; }
void ConfigManager::setLanguage(const QString &v) { m_language = (v == QStringLiteral("zh_CN") ? v : QStringLiteral("en")); }
```

- [ ] **Step 3: Load and save the new keys**

In `load()`, after `m_showGrid = obj.value("showGrid").toBool(m_showGrid);`, add:

```cpp
    m_nightMode = obj.value("nightMode").toString(m_nightMode) == QStringLiteral("texture")
                      ? QStringLiteral("texture") : QStringLiteral("simple");
    m_language  = obj.value("language").toString(m_language) == QStringLiteral("zh_CN")
                      ? QStringLiteral("zh_CN") : QStringLiteral("en");
```

In `save()`, after `o["showGrid"] = m_showGrid;`, add:

```cpp
    o["nightMode"] = m_nightMode;
    o["language"]  = m_language;
```

- [ ] **Step 4: Update the config round-trip test**

In `tests/test_configmanager.cpp`, inside `savesAndLoadsValues()`, after `c.setShowGrid(true);`, add:

```cpp
        c.setNightMode(QStringLiteral("texture"));
        c.setLanguage(QStringLiteral("zh_CN"));
```

Inside the same test, before the closing brace, add:

```cpp
        QCOMPARE(c2.nightMode(), QStringLiteral("texture"));
        QCOMPARE(c2.language(), QStringLiteral("zh_CN"));
```

- [ ] **Step 5: Run the config test**

Run:

```bash
cmake --build build --target test_configmanager
cd build && ctest -R test_configmanager -V
```

Expected: PASS.

---

### Task 2: Add night-mode uniform to renderer and shader

**Files:**
- Modify: `src/globe/render/GlobeRenderer.h`
- Modify: `src/globe/render/GlobeRenderer.cpp`
- Modify: `src/globe/render/shaders/earth.frag`

- [ ] **Step 1: Add renderer API in `GlobeRenderer.h`**

After `void setShowGrid(bool v);`, add:

```cpp
    void setUseNightTexture(bool v);
```

After `bool m_showGrid = false;`, add:

```cpp
    bool m_useNightTexture = false;
```

- [ ] **Step 2: Implement in `GlobeRenderer.cpp`**

After `void GlobeRenderer::setShowGrid(bool v) { m_showGrid = v; }`, add:

```cpp
void GlobeRenderer::setUseNightTexture(bool v) { m_useNightTexture = v; }
```

In `render()`, after `m_earthProg->setUniformValue("uShowGrid", m_showGrid ? 1.0f : 0.0f);`, add:

```cpp
    m_earthProg->setUniformValue("uUseNightTexture", m_useNightTexture ? 1.0f : 0.0f);
```

- [ ] **Step 3: Update `earth.frag`**

Add the uniform declaration after `uShowGrid`:

```glsl
uniform float uUseNightTexture; // 1 = use night texture, 0 = simple dim day map
```

Change the night texture sampling from:

```glsl
    vec3 night = (uHasNight > 0.5) ? texture(uNight, uv).rgb : vec3(0.0);
```

to:

```glsl
    vec3 night = (uHasNight > 0.5 && uUseNightTexture > 0.5)
                     ? texture(uNight, uv).rgb : vec3(0.0);
```

- [ ] **Step 4: Build the library**

Run:

```bash
cmake --build build --target globe_lib
```

Expected: compiles cleanly.

---

### Task 3: Create SettingsDialog

**Files:**
- Create: `src/globe/SettingsDialog.h`
- Create: `src/globe/SettingsDialog.cpp`

- [ ] **Step 1: Write `src/globe/SettingsDialog.h`**

```cpp
#pragma once
#include <QDialog>

class ConfigManager;
class QCheckBox;
class QRadioButton;
class QComboBox;

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(ConfigManager *config, QWidget *parent = nullptr);

signals:
    void settingsChanged();

private slots:
    void accept() override;

private:
    ConfigManager *m_config = nullptr;
    QCheckBox *m_gridCheck = nullptr;
    QRadioButton *m_simpleNightRadio = nullptr;
    QRadioButton *m_textureNightRadio = nullptr;
    QComboBox *m_languageCombo = nullptr;

    void setupUi();
};
```

- [ ] **Step 2: Write `src/globe/SettingsDialog.cpp`**

```cpp
#include "SettingsDialog.h"
#include "ConfigManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QCheckBox>
#include <QRadioButton>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QDialogButtonBox>

SettingsDialog::SettingsDialog(ConfigManager *config, QWidget *parent)
    : QDialog(parent), m_config(config) {
    setWindowTitle(tr("Settings"));
    setupUi();
}

void SettingsDialog::setupUi() {
    auto *layout = new QVBoxLayout(this);

    auto *form = new QFormLayout;

    m_languageCombo = new QComboBox;
    m_languageCombo->addItem(tr("English"), QStringLiteral("en"));
    m_languageCombo->addItem(QStringLiteral(u"\u7b80\u4f53\u4e2d\u6587"), QStringLiteral("zh_CN"));
    const int langIndex = m_languageCombo->findData(m_config->language());
    m_languageCombo->setCurrentIndex(langIndex >= 0 ? langIndex : 0);
    form->addRow(tr("Language:"), m_languageCombo);

    m_gridCheck = new QCheckBox(tr("Show Grid"));
    m_gridCheck->setChecked(m_config->showGrid());
    form->addRow(m_gridCheck);

    auto *nightGroup = new QGroupBox(tr("Night Mode"));
    auto *nightLayout = new QVBoxLayout(nightGroup);
    m_simpleNightRadio = new QRadioButton(tr("Simple Night"));
    m_textureNightRadio = new QRadioButton(tr("Realistic Night (city lights)"));
    nightLayout->addWidget(m_simpleNightRadio);
    nightLayout->addWidget(m_textureNightRadio);
    if (m_config->nightMode() == QStringLiteral("texture"))
        m_textureNightRadio->setChecked(true);
    else
        m_simpleNightRadio->setChecked(true);
    form->addRow(nightGroup);

    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

void SettingsDialog::accept() {
    if (!m_config) { QDialog::accept(); return; }

    m_config->setLanguage(m_languageCombo->currentData().toString());
    m_config->setShowGrid(m_gridCheck->isChecked());
    m_config->setNightMode(m_textureNightRadio->isChecked()
                               ? QStringLiteral("texture") : QStringLiteral("simple"));
    m_config->save();
    emit settingsChanged();
    QDialog::accept();
}
```

Note: the Chinese label `简体中文` is embedded via Unicode escape (`u"\u7b80\u4f53\u4e2d\u6587"`) to keep the source file ASCII-safe. The `tr()` wrapper on the English entries still allows translation if needed.

- [ ] **Step 3: Add `SettingsDialog` to `CMakeLists.txt`**

In `qt_add_library(globe_lib ...)`, add after `src/globe/LocationProvider.cpp`:

```cmake
    src/globe/SettingsDialog.h
    src/globe/SettingsDialog.cpp
```

- [ ] **Step 4: Build the library**

Run:

```bash
cmake --build build --target globe_lib
```

Expected: compiles cleanly.

---

### Task 4: Update the tray menu

**Files:**
- Modify: `src/globe/SystemTray.h`
- Modify: `src/globe/SystemTray.cpp`

- [ ] **Step 1: Replace Show Grid signal with Settings in `SystemTray.h`**

Change:

```cpp
    void toggleShowGrid(bool on);
```

to:

```cpp
    void openSettings();
```

Remove the `setShowGridChecked` declaration and the `m_showGridAction` member.

The header should look like:

```cpp
#pragma once
#include <QSystemTrayIcon>

class SystemTray : public QSystemTrayIcon {
    Q_OBJECT
public:
    explicit SystemTray(QObject *parent = nullptr);
    void setSolarTooltip(const QString &text);
signals:
    void toggleVisibility();
    void resetView();
    void centerOnMe();
    void openSettings();
    void quit();
};
```

- [ ] **Step 2: Replace Show Grid action with Settings in `SystemTray.cpp`**

Replace the Show Grid block:

```cpp
    m_showGridAction = menu->addAction(tr("Show Grid"));
    m_showGridAction->setCheckable(true);
    connect(m_showGridAction, &QAction::toggled, this, &SystemTray::toggleShowGrid);
```

with:

```cpp
    menu->addAction(tr("Settings..."), this, &SystemTray::openSettings);
```

Remove the `#include <QAction>` if it is no longer needed (it still is for `menu->addAction` return values and the Quit action, so keep it).

Remove the `setShowGridChecked` method body at the bottom of the file.

- [ ] **Step 3: Build the library**

Run:

```bash
cmake --build build --target globe_lib
```

Expected: compiles cleanly.

---

### Task 5: Wire SettingsDialog and translator in main.cpp

**Files:**
- Modify: `main.cpp`
- Modify: `globe_zh_CN.ts`

- [ ] **Step 1: Install translator at startup**

After `QApplication app(argc, argv);`, add:

```cpp
    QTranslator translator;
    {
        ConfigManager tmpConfig;
        const QString lang = tmpConfig.language();
        if (lang == QStringLiteral("zh_CN")) {
            if (translator.load(QStringLiteral(":/i18n/globe_zh_CN.qm")))
                app.installTranslator(&translator);
        }
    }
```

Then keep the existing `ConfigManager config;` line.

- [ ] **Step 2: Initialize renderer night mode**

After `w.view()->renderer().setShowGrid(config.showGrid());`, add:

```cpp
    w.view()->renderer().setUseNightTexture(config.nightMode() == QStringLiteral("texture"));
```

- [ ] **Step 3: Replace the old tray Show Grid connection with Settings**

Remove the block:

```cpp
    tray.setShowGridChecked(config.showGrid());
    ...
    QObject::connect(&tray, &SystemTray::toggleShowGrid, &w,
        [&w, &config](bool on) {
            config.setShowGrid(on);
            config.save();
            w.view()->renderer().setShowGrid(on);
            w.view()->update();
        });
```

Replace it with a single `SettingsDialog` instance and connection:

```cpp
    SettingsDialog settingsDialog(&config, &w);
    QObject::connect(&settingsDialog, &SettingsDialog::settingsChanged, &w,
        [&w, &config, &translator, &app]() {
            const bool useTexture = config.nightMode() == QStringLiteral("texture");
            w.view()->renderer().setShowGrid(config.showGrid());
            w.view()->renderer().setUseNightTexture(useTexture);
            // Reload translator if language changed.
            app.removeTranslator(&translator);
            if (config.language() == QStringLiteral("zh_CN")) {
                if (translator.load(QStringLiteral(":/i18n/globe_zh_CN.qm")))
                    app.installTranslator(&translator);
            }
            w.view()->update();
        });
    QObject::connect(&tray, &SystemTray::openSettings, &w,
        [&settingsDialog]() { settingsDialog.exec(); });
```

Add `#include "globe/SettingsDialog.h"` near the other globe includes at the top of `main.cpp`.

Add `#include <QTranslator>` to the Qt includes.

- [ ] **Step 4: Build the app**

Run:

```bash
cmake --build build --target globe
```

Expected: compiles cleanly.

- [ ] **Step 5: Add Chinese translations to `globe_zh_CN.ts`**

Update `globe_zh_CN.ts`:

```xml
<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh_CN">
<context>
    <name>SettingsDialog</name>
    <message>
        <location filename="src/globe/SettingsDialog.cpp"/>
        <source>Settings</source>
        <translation>设置</translation>
    </message>
    <message>
        <location filename="src/globe/SettingsDialog.cpp"/>
        <source>Language:</source>
        <translation>语言：</translation>
    </message>
    <message>
        <location filename="src/globe/SettingsDialog.cpp"/>
        <source>English</source>
        <translation>English</translation>
    </message>
    <message>
        <location filename="src/globe/SettingsDialog.cpp"/>
        <source>Show Grid</source>
        <translation>显示网格</translation>
    </message>
    <message>
        <location filename="src/globe/SettingsDialog.cpp"/>
        <source>Night Mode</source>
        <translation>夜间模式</translation>
    </message>
    <message>
        <location filename="src/globe/SettingsDialog.cpp"/>
        <source>Simple Night</source>
        <translation>简单夜间</translation>
    </message>
    <message>
        <location filename="src/globe/SettingsDialog.cpp"/>
        <source>Realistic Night (city lights)</source>
        <translation>真实夜间（城市灯光）</translation>
    </message>
</context>
<context>
    <name>SystemTray</name>
    <message>
        <location filename="src/globe/SystemTray.cpp"/>
        <source>Hide/Show</source>
        <translation>隐藏/显示</translation>
    </message>
    <message>
        <location filename="src/globe/SystemTray.cpp"/>
        <source>Reset View</source>
        <translation>重置视图</translation>
    </message>
    <message>
        <location filename="src/globe/SystemTray.cpp"/>
        <source>Center on Me</source>
        <translation>定位到我</translation>
    </message>
    <message>
        <location filename="src/globe/SystemTray.cpp"/>
        <source>Settings...</source>
        <translation>设置...</translation>
    </message>
    <message>
        <location filename="src/globe/SystemTray.cpp"/>
        <source>About</source>
        <translation>关于</translation>
    </message>
    <message>
        <location filename="src/globe/SystemTray.cpp"/>
        <source>Quit</source>
        <translation>退出</translation>
    </message>
</context>
<context>
    <name>GlobeWindow</name>
    <message>
        <location filename="src/globe/GlobeWindow.cpp"/>
        <source>Sun sub-point: %1°, %2°</source>
        <translation>太阳直射点：%1°，%2°</translation>
    </message>
</context>
</TS>
```

The `location` filenames should match the actual source files. Adjust line numbers are not required for runtime; Qt Linguist uses them as hints.

- [ ] **Step 6: Regenerate translations and rebuild**

Run:

```bash
cmake --build build --target update_translations
cmake --build build --target globe
```

Expected: `.qm` file generated, app links.

---

### Task 6: Verify and commit

**Files:**
- All of the above

- [ ] **Step 1: Run the test suite**

Run:

```bash
cd build && ctest --output-on-failure
```

Expected: all tests pass.

- [ ] **Step 2: Manual smoke test**

Run `build/globe.exe`:

1. Right-click tray → **Settings...**
2. Toggle **Show Grid** → OK; grid appears.
3. Open Settings again, select **Realistic Night (city lights)** → OK; city lights appear on the night side.
4. Switch to **Simple Night** → OK; city lights disappear, night side is dim land/sea.
5. Switch language to **简体中文** → OK; dialog and tray menu show Chinese.

- [ ] **Step 3: Commit**

```bash
git add src/globe/ConfigManager.h src/globe/ConfigManager.cpp

git add src/globe/SettingsDialog.h src/globe/SettingsDialog.cpp

git add src/globe/SystemTray.h src/globe/SystemTray.cpp

git add src/globe/render/GlobeRenderer.h src/globe/render/GlobeRenderer.cpp

git add src/globe/render/shaders/earth.frag

git add main.cpp CMakeLists.txt globe_zh_CN.ts

git add tests/test_configmanager.cpp

git add docs/superpowers/specs/2026-06-19-night-mode-design.md

git add docs/superpowers/plans/2026-06-19-night-mode.md

git commit -m "feat: add Settings dialog with night mode and i18n"
```

---

## Self-Review

1. **Spec coverage:**
   - Settings dialog from tray menu → Task 3 + Task 4.
   - Grid checkbox in dialog → Task 3.
   - Night mode radio group → Task 3.
   - Simple vs realistic night → Task 2 + Task 3.
   - i18n English + zh_CN → Task 5.
   - Config persistence → Task 1.

2. **Placeholder scan:** No TBD/TODO/"implement later"; all code and commands are explicit.

3. **Type consistency:**
   - `nightMode()` returns `QString`, values `"simple"`/`"texture"`.
   - `language()` returns `QString`, values `"en"`/`"zh_CN"`.
   - `setUseNightTexture(bool)` matches `m_useNightTexture` and the shader uniform.
   - `SettingsDialog` emits `settingsChanged()` exactly once on OK.

No gaps found.
