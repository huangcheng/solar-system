# Location Selector — Design Spec (flat-map feature addendum)

**Date:** 2026-06-20
**Status:** Draft — awaiting user review, then implementation plan
**Parent:** `docs/superpowers/specs/2026-06-20-flat-map-mode-design.md` (this refines the home-marker location UX).

## Goal

Replace the manual Latitude/Longitude spinboxes with a **human-friendly, robust location setter** for the home marker: a searchable **city selector**, a **detect button** (system GPS/WiFi, or approximate IP with multi-service fallback), and an optional **auto-detect on startup**. All three set the marker; the user never types coordinates.

## Motivation

- Typing lat/lon is unfriendly and error-prone.
- A single IP service is fragile (outages, rate limits); a fallback chain is needed.
- System (Qt Positioning) is accurate but only works where an OS backend exists (now available via the MSVC/winrt build).
- A bundled city list gives an offline, always-available fallback.

## Non-Goals

- Continuous live tracking / movement (one-shot + optional startup detect only).
- Reverse-geocoding a detected coordinate to a city name for display (the coord label is enough).
- Map-based location picking.
- Saving per-city custom offsets.

## Architecture

Three location sources feed one outcome (`config.homeLatitude/homeLongitude` → marker). All are async except the city selector (instant, offline).

```
src/celestial/
├── CityDatabase.h/.cpp        (NEW — loads bundled cities.json; lookup by "City, Country")
├── IpLocator.h/.cpp           (NEW — QNetworkAccessManager; tries ip-api → ipinfo → ipapi.co)
├── LocationProvider.h/.cpp    (modified — add requestOnceSystem(): one-shot Qt Positioning)
├── SettingsDialog.h/.cpp      (modified — city combo + detect button + auto-start + coord label; remove spinboxes)
├── ConfigManager.h/.cpp       (modified — add autoDetectOnStart; homeLat/Lon become set-only)
├── data/cities.json           (NEW resource — ~2000 cities, pop > 500k)
├── data/ip-services.json      (NEW — loose file, ordered IP-geo services + field map; editable post-ship, no recompile)
src/apps/earth/main.cpp        (modified — wire detect results + startup auto-detect)
```

## Components

### 1. `CityDatabase` (offline)
- Loads a bundled JSON resource `:/data/cities.json` (or `data/cities.json` on disk) of `{name, country, lat, lon}` for ~2000 cities (population > 500k).
- Source: SimpleMaps *worldcities basic* (free, attribution), filtered to population ≥ 500000.
- `QList<CityEntry> search(const QString&)` / a model for a searchable combo (city + country).
- `std::optional<CityEntry> findExact("City, Country")`.

### 2. `IpLocator` (network, fallback chain — **data-driven, not hardcoded**)
- One public method `request()`; emits `located(double lat, double lon, const QString &city)` or `failed()`.
- Reads the ordered service list from a **bundled `data/ip-services.json`** — a loose file shipped next to the exe, so it can be **edited or replaced without recompiling or reshipping the binary**. Each entry: `{name, url, latField, lonField, cityField, timeoutSec}`. (Services with a combined `"loc":"lat,lon"` field use `locField` + `locSeparator` instead of separate lat/lon fields.) `IpLocator` iterates the list in file order, each entry's timeout (default 3 s), and the first valid JSON with lat/lon wins.
- Default `data/ip-services.json` ships `ip-api → ipinfo → ipapi.co` as a starting set. A **compiled-in minimal default** is used only if the loose file is missing/corrupt (so deleting the file never breaks the app).
- Uses `QNetworkAccessManager`. Cancel pending + stop on first success. No API keys.

### 3. `LocationProvider` (system) — add one-shot
- Add `requestOnceSystem(int timeoutMs = 8000)`: uses `QGeoPositionInfoSource::requestUpdate(timeout)`; on `positionUpdated` with a valid coord, emit `locationChanged`. (Keep existing `start()`/`stop()` for the auto-start continuous case — or switch auto-start to one-shot too; prefer one-shot to avoid continuous tracking.)

### 4. `ConfigManager`
- Add `bool autoDetectOnStart() / setAutoDetectOnStart(bool)` (default **false**), persisted (`"autoDetectOnStart"`).
- `homeLatitude/homeLongitude` remain, but are now **set programmatically** (no UI spinboxes). Default 0,0; the marker only shows when `showHomeMarker` is on and a location has been set.

### 5. `SettingsDialog` UI
- Remove the Lat/Lon spinboxes and the "Auto-detect my location" checkbox.
- Keep **"Show home marker"** checkbox.
- **City selector:** editable `QComboBox` with `QCompleter` (popup, "City, Country"). Selecting → set homeLat/Lon + check Show-marker + update coord label.
- **"Detect Location" button** → `QMenu`: "System (GPS/WiFi)" and "IP (approximate)". Shows "Detecting…" while in flight; on success updates coords + Show-marker + label; on failure shows a brief error in the label.
- **"Auto-detect on startup"** checkbox (bound to `autoDetectOnStart`, default off).
- **Coord label:** read-only, shows e.g. `39.9042°, 116.4074°  (Beijing)` or `not set`.

### 6. `main.cpp` wiring
- Startup: `if (config.autoDetectOnStart()) location.requestOnceSystem();`
- The detect button(s) in the dialog call `LocationProvider::requestOnceSystem()` / `IpLocator::request()` directly (dialog owns the async handles); on result, write config + `widget.setHomeLocation()` + update the dialog's label.
- Existing `locationChanged` path (real system fixes) → persist + `setHomeLocation`.

## Data Flow

City pick / System detect / IP detect → `config.setHomeLatitude/Longitude` + `config.save()` + `widget.setHomeLocation(lat,lon)` + dialog coord-label update. Marker visibility = `config.showHomeMarker()` (a successful detect auto-checks it). `applyOptionsFromConfig()` still gates `hasHome` on `showHomeMarker`.

## Design Decisions (resolved)

| Question | Decision |
|----------|----------|
| Manual lat/lon input | Removed — replaced by city selector |
| City list size | ~2000 (population > 500k), from SimpleMaps worldcities |
| IP services | **Config-file-driven** (`data/ip-services.json`, loose/editable, no recompile to change): default chain ip-api → ipinfo → ipapi.co (3 s each); compiled-in minimal fallback if the file is missing/corrupt |
| Auto-detect on startup | Off by default |
| Continuous tracking | No — one-shot detects only (startup auto-detect is one-shot) |
| Marker visibility | `showHomeMarker`; a successful detect auto-enables it |

## Edge Cases
- VPN/proxy → IP result wrong; label says "approximate", user can override via city selector.
- No network / all IP services fail → "Detect failed" in label; city selector still works offline.
- No OS location backend → system detect fails gracefully; city/IP still work.
- City list missing/empty resource → selector empty but app doesn't crash.
- `ip-services.json` missing/corrupt → `IpLocator` uses the compiled-in minimal default (no crash; still tries at least one service).

## Testing
- Unit: `CityDatabase` loads + `findExact` returns correct coords for a known city.
- Unit: `IpLocator` parsing with canned JSON (no network) — verify each service's field extraction, including the combined `loc` form.
- Unit: `IpLocator` reads `ip-services.json` and falls back to the compiled-in default when the file is missing/corrupt.
- Unit: `ConfigManager.autoDetectOnStart` round-trip.
- Visual: each of the 3 sources sets the marker; auto-start triggers system detect on launch.

## Open Questions
None — all resolved.

## Next Step
After user review, invoke `writing-plans` for the implementation plan.
