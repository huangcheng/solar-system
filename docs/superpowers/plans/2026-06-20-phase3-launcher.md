# Phase 3 — solar.exe Launcher Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: superpowers:subagent-driven-development or superpowers:executing-plans. Steps use checkbox (`- [ ]`) syntax.

**Goal:** Build `solar.exe` — a control-panel application that lists every body, lets the user add/remove each as a desktop widget (by spawning the corresponding body executable), and has an "Exit All" action.

**Architecture:** A small `QWidget` launcher window built on top of the `celestial` library. It uses `QProcess` to spawn/terminate body executables located next to itself. Tracks live processes in a map keyed by `bodyId`. Singleton mutex `GlobeAppSingleton_Solar_v1`.

**Tech Stack:** Qt6 Widgets, QProcess, celestial library.

---

## File Structure

```
src/
├── apps/
│   └── solar/
│       ├── main.cpp              (NEW)
│       ├── LauncherWindow.h/.cpp (NEW)
└── celestial/
    └── (unchanged)
```

---

## Task 1: `LauncherWindow` skeleton

**Files:**
- Create: `src/apps/solar/LauncherWindow.h`
- Create: `src/apps/solar/LauncherWindow.cpp`

- [ ] **Step 1: `LauncherWindow.h`**

```cpp
#pragma once
#include <QWidget>
#include <QMap>
#include <QString>
class QProcess;
class QCheckBox;
class QLabel;

class LauncherWindow : public QWidget {
    Q_OBJECT
public:
    explicit LauncherWindow(QWidget *parent = nullptr);
    ~LauncherWindow() override;
private slots:
    void toggleBody(const QString& bodyId, bool on);
    void exitAll();
private:
    struct BodyEntry { QString id; QString name; QCheckBox* check = nullptr; QLabel* status = nullptr; };
    QList<BodyEntry> m_entries;
    QMap<QString, QProcess*> m_processes;

    QString exePath(const QString& bodyId) const;
    void spawn(const QString& bodyId);
    void kill(const QString& bodyId);
    void buildUi();
};
```

- [ ] **Step 2: `LauncherWindow.cpp`**

- `exePath(bodyId)` returns `QCoreApplication::applicationDirPath() + "/" + bodyId + ".exe"` (Windows) / `bodyId` elsewhere.
- `buildUi()` creates a vertical layout: a header label, one row per body (`QCheckBox` + `QLabel` status), an "Exit All" `QPushButton`, a close button. Bodies enumerated from a static list matching the CMake `BODIES`.
- `toggleBody`: if on → `spawn`, else → `kill`.
- `spawn`: create `QProcess`, `start(exePath)`, connect `finished` to update status label ("running"/"stopped"). Guard against duplicate spawn.
- `kill`: `process->terminate()` then `deleteLater` after finished; remove from map.
- `exitAll`: iterate map and kill each.
- Destructor: `exitAll()`.

- [ ] **Step 3: Commit**

```bash
git add -A
git commit -m "feat(solar): LauncherWindow skeleton"
```

---

## Task 2: `solar` main + CMake target + singleton

**Files:**
- Create: `src/apps/solar/main.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: `main.cpp`**

```cpp
#include "LauncherWindow.h"
#include <QApplication>
#include <QSharedMemory>
#ifdef Q_OS_WIN
#include <windows.h>
#endif
int main(int argc, char *argv[]) {
#ifdef Q_OS_WIN
    {
        HANDLE m = CreateMutexW(nullptr, TRUE, L"GlobeAppSingleton_Solar_v1");
        if (GetLastError() == ERROR_ALREADY_EXISTS) { if (m) CloseHandle(m); return 0; }
    }
#else
    { QSharedMemory shm(QStringLiteral("GlobeAppSingleton_Solar_v1")); if (!shm.create(1)) return 0; }
#endif
    QApplication app(argc, argv);
    app.setApplicationName("Solar");
    LauncherWindow w;
    w.show();
    return app.exec();
}
```

- [ ] **Step 2: Add `solar` target to CMake**

```cmake
qt_add_executable(solar WIN32 MACOSX_BUNDLE
    src/apps/solar/main.cpp
    src/apps/solar/LauncherWindow.h
    src/apps/solar/LauncherWindow.cpp
)
target_link_libraries(solar PRIVATE celestial)
```

(Place after the `BODIES` loop; `solar` is not a body.)

- [ ] **Step 3: Build**

```bash
cmake --build F:\globe\build --target solar 2>&1 | tail -10
```

- [ ] **Step 4: Smoke test**

```bash
F:\globe\build\solar.exe
```

Check: window shows 10 body rows; checking "Sun" spawns `sun.exe`; unchecking kills it; "Exit All" closes all spawned widgets. Then close solar.

- [ ] **Step 5: Commit + tag**

```bash
git add -A
git commit -m "feat(solar): launcher panel to add/remove body widgets"
git tag phase3-launcher
```

---

## Notes

- This phase does NOT yet implement Solar System Mode (Phase 4). The launcher's "Solar System Mode" button is added in Phase 4.
- Translation: add `tr()` to all user-visible strings; defer `.ts` update to Phase 5.
