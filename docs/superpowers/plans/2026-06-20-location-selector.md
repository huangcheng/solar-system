# Location Selector Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: superpowers:subagent-driven-development or superpowers:executing-plans. Steps use checkbox (`- [ ]`) syntax.

**Goal:** Replace the manual lat/lon spinboxes with a human-friendly location setter: a searchable **city selector** (~2000 cities), a **detect button** (system GPS/WiFi via Qt Positioning, or approximate IP via a **config-file-driven** service chain with fallback), and an optional **auto-detect on startup**.

**Architecture:** Three async/offline sources (`CityDatabase`, `IpLocator`, `LocationProvider::requestOnceSystem`) all funnel into `config.homeLat/Lon` + `showHomeMarker` → the existing marker render path. IP services are read from a loose `data/ip-services.json` (editable post-ship, no recompile) with a compiled-in fallback. Builds on the existing `feat/flat-map` work (MSVC kit).

**Tech Stack:** Qt6 (Positioning, Network), CMake/MSVC/Ninja, existing `celestial` library.

---

## File Structure

```
src/celestial/
├── CityDatabase.h/.cpp        (NEW)
├── IpLocator.h/.cpp           (NEW — QNetworkAccessManager, config-driven)
├── LocationProvider.h/.cpp    (modified — + requestOnceSystem())
├── ConfigManager.h/.cpp       (modified — + autoDetectOnStart)
├── SettingsDialog.h/.cpp      (modified — city combo + detect btn + auto-start + coord label; remove spinboxes)
├── data/
│   ├── cities.json            (NEW — Qt resource, ~2000 cities >500k)
│   └── ip-services.json       (NEW — LOOSE file, editable post-ship)
└── cities.qrc                 (NEW — bundles cities.json as :/data/cities.json)
src/apps/earth/main.cpp        (modified — wire detect + startup auto-detect)
tests/
├── test_citydatabase.cpp      (NEW)
├── test_iplocator.cpp         (NEW — canned JSON, no network)
└── CMakeLists.txt             (modified)
CMakeLists.txt                 (modified — Qt::Network, new sources, cities.qrc, install ip-services.json)
```

Build/test (MSVC) — run inside a vcvars shell:
```bash
cmd /c "call `"C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat`" >nul && set PATH=C:\Qt\6.11.1\msvc2022_64\bin;C:\Qt\Tools\CMake_64\bin;%PATH% && <command>"
```
Worktree: `F:\globe\.slim\worktrees\flat-map`, MSVC build dir `build-msvc`.

---

## Task 1: City data — fetch SimpleMaps, filter to >500k, produce `cities.json`

**Files:** Create `src/celestial/data/cities.json` + `src/celestial/cities.qrc`.

- [ ] **Step 1: Fetch + filter the dataset.** Download SimpleMaps worldcities basic (free, attribution), unzip, filter population ≥ 500000, emit `src/celestial/data/cities.json` as an array of `{name, country, lat, lon}`. PowerShell:
```powershell
$tmp = "$env:TEMP\opencode\sm"
New-Item -ItemType Directory -Force $tmp | Out-Null
$zip = "$tmp\worldcities.zip"
Invoke-WebRequest "https://simplemaps.com/static/data/world-cities/basic/simplemaps_worldcities_basicv1.75.zip" -OutFile $zip
Expand-Archive $zip -DestinationPath $tmp -Force
$csv = Get-ChildItem "$tmp\worldcities.csv" | Select-Object -First 1
# CSV cols: city, city_ascii, lat, lng, country, ..., population
Import-Csv $csv.FullName | Where-Object { [double]$_.population -ge 500000 } |
  Select-Object @{n='name';e={$_.city_ascii}}, @{n='country';e={$_.country}}, @{n='lat';e={[double]$_.lat}}, @{n='lon';e={[double]$_.lng}} |
  ConvertTo-Json -Compress | Out-File "F:\globe\.slim\worktrees\flat-map\src\celestial\data\cities.json" -Encoding UTF8
(Get-Content "F:\globe\.slim\worktrees\flat-map\src\celestial\data\cities.json" | ConvertFrom-Json).Count
```
Expected: ~1500–2000 entries. If the URL 404s (version drift), fall back to the latest from `https://simplemaps.com/data/world-cities` (basic download). Add an `# Attribution: SimpleMaps worldcities (free)` comment is NOT legal in raw JSON — instead record attribution in a sibling `cities.LICENSE.txt` and the README.

- [ ] **Step 2: Create `src/celestial/cities.qrc`** bundling it as `:/data/cities.json`:
```xml
<RCC><qresource prefix="/data"><file>data/cities.json</file></qresource></RCC>
```

- [ ] **Step 3: Commit**
```bash
git add src/celestial/data/cities.json src/celestial/cities.qrc src/celestial/data/cities.LICENSE.txt
git commit -m "assets: bundled ~2000 world cities (>500k) from SimpleMaps as :/data/cities.json"
```

---

## Task 2: `CityDatabase` (TDD)

**Files:** Create `src/celestial/CityDatabase.h/.cpp`; create `tests/test_citydatabase.cpp`; modify `CMakeLists.txt` (add sources + Qt::Core) + `tests/CMakeLists.txt`.

- [ ] **Step 1: `CityDatabase.h`**
```cpp
#pragma once
#include <QString>
#include <QList>
#include <optional>
struct CityEntry { QString name; QString country; double lat; double lon; QString display() const { return name + ", " + country; } };
class CityDatabase {
public:
    static CityDatabase &instance();           // loads :/data/cities.json once
    const QList<CityEntry> &all() const { return m_cities; }
    std::optional<CityEntry> findByDisplay(const QString &display) const;  // "City, Country"
private:
    CityDatabase();
    QList<CityEntry> m_cities;
};
```

- [ ] **Step 2: `CityDatabase.cpp`** — load `:/data/cities.json` via `QFile` + `QJsonDocument`; parse the array; populate `m_cities`. (For tests, add a constructor-from-bytes or make the parse a static free function `parseCities(const QJsonArray&)` so it's unit-testable without the resource.)

- [ ] **Step 3: `test_citydatabase.cpp`** — (a) `parseCities` of a tiny canned array returns expected entries; (b) `instance().all()` is non-empty (resource loads); (c) `findByDisplay("Tokyo, Japan")` returns a valid entry with sane lat/lon.

- [ ] **Step 4: CMake** — add `CityDatabase.h/.cpp` to the `celestial` target; add the `cities.qrc` via `qt_add_resources(celestial PREFIX "/data" FILES data/cities.json)` (or `target_sources(celestial PRIVATE cities.qrc)` — AUTORCC handles it). Register `test_citydatabase`.

- [ ] **Step 5: Build + test + commit.** Commit: `feat: CityDatabase loads bundled cities.json + tests`.

---

## Task 3: `ip-services.json` default + `IpLocator` (TDD, config-driven)

**Files:** Create `src/celestial/data/ip-services.json`; create `src/celestial/IpLocator.h/.cpp`; create `tests/test_iplocator.cpp`; modify `CMakeLists.txt` (add `Qt::Network` to celestial, new sources, install the loose json) + `tests/CMakeLists.txt`.

- [ ] **Step 1: `src/celestial/data/ip-services.json`** (the shipped default, loose/editable):
```json
{
  "services": [
    { "name": "ip-api",  "url": "http://ip-api.com/json/",  "latField": "lat",      "lonField": "lon",     "cityField": "city",     "timeoutSec": 3 },
    { "name": "ipinfo",  "url": "https://ipinfo.io/json/",  "locField": "loc",      "locSeparator": ",",   "cityField": "city",     "timeoutSec": 3 },
    { "name": "ipapi",   "url": "https://ipapi.co/json/",    "latField": "latitude", "lonField": "longitude","cityField": "city",     "timeoutSec": 3 }
  ]
}
```

- [ ] **Step 2: `IpLocator.h`**
```cpp
#pragma once
#include <QObject>
#include <QNetworkAccessManager>
struct IpService { QString name, url, latField, lonField, locField, locSeparator, cityField; int timeoutSec = 3; };
class QNetworkReply;
class IpLocator : public QObject {
    Q_OBJECT
public:
    explicit IpLocator(QObject *parent = nullptr);
    void request();   // try services in order from data/ip-services.json (fallback: compiled-in default)
signals:
    void located(double lat, double lon, const QString &city);
    void failed();
private:
    QNetworkAccessManager m_nam;
    QList<IpService> m_services;
    int m_index = 0;
    void tryNext();
    void parseReply(QNetworkReply *r, const IpService &s);  // extract lat/lon per field map (locField+sep OR lat/lon)
    static QList<IpService> loadServices();        // reads appDir/data/ip-services.json; compiled-in fallback
    static QList<IpService> compiledDefault();
};
```

- [ ] **Step 3: `IpLocator.cpp`** — `loadServices()` reads `QCoreApplication::applicationDirPath() + "/data/ip-services.json"`; on missing/invalid → `compiledDefault()` (a one-entry ip-api). `request()` sets `m_index=0; tryNext()`. `tryNext()` GETs `m_services[m_index].url` with a `QTimer::singleShot(timeoutSec*1000)` to abort+advance; on reply, `parseReply`; on valid coords → `emit located` + stop; on failure → `++m_index; tryNext()`; past end → `emit failed`. `parseReply` handles both the separate latField/lonField case and the combined `locField`+separator case.

- [ ] **Step 4: Refactor parsing for testability** — make `static bool IpLocator::extract(const QJsonDocument &doc, const IpService &s, double &outLat, double &outLon, QString &outCity)` so tests can feed canned JSON per service shape.

- [ ] **Step 5: `test_iplocator.cpp`** — verify `extract` against canned ip-api JSON, ipinfo JSON (loc field), and ipapi JSON; verify `loadServices` returns ≥1 service; verify `compiledDefault()` is non-empty (covers the missing-file fallback).

- [ ] **Step 6: CMake** — add `find_package` already includes Network? Add `Qt6::Network` to `celestial`'s `target_link_libraries`; add `IpLocator.h/.cpp` to sources; `install(FILES src/celestial/data/ip-services.json DESTINATION data)` + a custom command to copy it into the build dir next to the exe so dev runs find it. Register `test_iplocator`.

- [ ] **Step 7: Build + test + commit.** Commit: `feat: IpLocator — config-driven IP geolocation with fallback (ip-services.json) + tests`.

---

## Task 4: `LocationProvider::requestOnceSystem()` (one-shot)

**Files:** Modify `src/celestial/LocationProvider.h/.cpp`.

- [ ] **Step 1:** Add `void requestOnceSystem(int timeoutMs = 8000);` — creates the default source if needed, calls `m_src->requestUpdate(timeoutMs)`; the existing `positionUpdated` lambda already emits `locationChanged` on a valid fix. (Keep `start()`/`stop()` — auto-start will use `requestOnceSystem` for a one-shot, not continuous.)

- [ ] **Step 2:** Build + commit. `feat: LocationProvider one-shot system location request`.

---

## Task 5: `ConfigManager.autoDetectOnStart` (TDD)

**Files:** Modify `src/celestial/ConfigManager.h/.cpp` + `tests/test_configmanager.cpp`.

- [ ] **Step 1 (test first):** add `testAutoDetectOnStart` — default false, set true, round-trips through save/load.
- [ ] **Step 2:** add field `bool m_autoDetectOnStart = false;`, getter/setter, `load()`/`save()` key `"autoDetectOnStart"`.
- [ ] **Step 3:** Build + test + commit. `feat(config): autoDetectOnStart setting`.

---

## Task 6: Settings UI redesign

**Files:** Modify `src/celestial/SettingsDialog.h/.cpp`.

- [ ] **Step 1: Header** — remove `m_latSpin`, `m_lonSpin`, `m_autoLocCheck`; add `QComboBox *m_cityCombo`, `QCompleter *m_cityCompleter`, `QPushButton *m_detectBtn`, `QMenu *m_detectMenu`, `QCheckBox *m_autoStartCheck`, `QLabel *m_coordLabel`; ctor now also takes `LocationProvider *location`; add a private `IpLocator m_ipLocator` member + slot `void onLocationFound(double lat, double lon, const QString &city)`.

- [ ] **Step 2: `setupUi()`** — populate `m_cityCombo` from `CityDatabase::instance().all()` (display strings); set editable + `m_cityCompleter` (popup, case-insensitive). Replace the lat/lon row with the city combo ("Home city:") + a "Detect Location" button (with a `QMenu`: "System (GPS/WiFi)", "IP (approximate)") + an "Auto-detect on startup" checkbox + a read-only coord label. Initialize label from `config.homeLatitude/homeLongitude`.

- [ ] **Step 3: Wire detects** — System action → `m_location->requestOnceSystem()`; IP action → `m_ipLocator.request()`. Connect `m_location->locationChanged` and `m_ipLocator::located` to `onLocationFound`. Connect `m_cityCombo::currentIndexChanged`/`activated` → look up `CityDatabase::findByDisplay(combo.currentText())` → if found, call `onLocationFound(lat, lon, city)`.

- [ ] **Step 4: `onLocationFound(lat, lon, city)`** — `m_config->setHomeLatitude(lat); setHomeLongitude(lon); setShowHomeMarker(true); save();` update `m_coordLabel` (e.g. `"39.9042°, 116.4074° (Beijing)"`); set the city combo to the matching city if present; `emit settingsChanged();` (so the globe/map marker updates live).

- [ ] **Step 5: `accept()`** — write `setAutoDetectOnStart(m_autoStartCheck->isChecked())`; keep writing viewMode/showHomeMarker/grid/etc. (Drop the old `setLocationOptIn` write — location opt-in is gone; the Detect button is on-demand.)

- [ ] **Step 6: `retranslateUi()`** — cover the new labels/button/menu items/checkbox.

- [ ] **Step 7:** Build + commit. `feat(ui): city selector + detect-location button + auto-start; remove manual lat/lon`.

---

## Task 7: `main.cpp` wiring

**Files:** Modify `src/apps/earth/main.cpp`.

- [ ] **Step 1:** Construct `SettingsDialog settingsDialog(&config, &widget, &location);` (new 3rd arg). 
- [ ] **Step 2:** Startup auto-detect — after `widget.applyOptionsFromConfig();`: `if (config.autoDetectOnStart()) location.requestOnceSystem();`
- [ ] **Step 3:** The existing `locationChanged` connection persists coords + `widget.setHomeLocation` (keep). The dialog's `settingsChanged` (already wired) → `applyOptionsFromConfig` re-reads `homeLat/Lon/showHomeMarker` → marker updates.
- [ ] **Step 4:** Build + commit. `feat(earth): wire auto-detect on startup + location-aware Settings`.

---

## Task 8: Verification

- [ ] **Step 1:** Build (MSVC), `ctest` (all green, incl. new tests). `windeployqt` + ensure `data/ip-services.json` is copied next to the exe.
- [ ] **Step 2:** Visual: open Settings → (a) pick a city → marker appears there on globe + map; (b) Detect → System → marker at real location (may prompt for OS permission); (c) Detect → IP → marker at ~IP location; (d) coord label updates each time; (e) Auto-detect on startup checked → relaunch → marker auto-sets via system.
- [ ] **Step 3:** Edit `data/ip-services.json` (reorder/remove a service) → IP detect uses the new set with no recompile (verifies the data-driven design).

---

## Spec Coverage
| Spec item | Task |
|-----------|------|
| ~2000 city DB + resource | 1 |
| `CityDatabase` + test | 2 |
| `ip-services.json` (loose, editable) + `IpLocator` (fallback) + test | 3 |
| `LocationProvider::requestOnceSystem` | 4 |
| `autoDetectOnStart` config | 5 |
| City selector + detect button + auto-start UI; remove spinboxes | 6 |
| Startup auto-detect + dialog wiring | 7 |
| Verification incl. data-driven IP edit | 8 |

## Notes
- **Qt::Network** must be linked (Task 3). `windeployqt` already ships Qt6Network.dll.
- `cities.json` = Qt resource (read-only); `ip-services.json` = loose file (editable). This matches the spec: edit IP services post-ship without recompile, but ship a fixed city list.
- No regression to globe path / flat-map mode / existing settings.
