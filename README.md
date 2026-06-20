# Globe

A tiny desktop Earth globe widget for Windows, with real-time day/night, a flat-map mode, and a growing solar-system view.

![App icon](src/celestial/data/icon.png)

## Features

- **3D Globe** — spins as a toy (up to 2880× real-time) with realistic day/night, city lights on the night side, and an optional grid.
- **Flat Map** — equirectangular world map with a live real-world terminator.
- **Location-aware** — pick a home city, detect your location via system GPS/WiFi or IP geolocation, and show a home marker on the globe/map.
- **System tray** — hide/show, reset view, center on home, settings, about, quit.
- **Settings** — language (English / 简体中文), view mode, night mode, spin speed, always-on-top, home location.
- **About dialog** — version, credits, license, and links.

## Build

### Requirements

- Windows 10/11
- Qt 6.5+ (tested with Qt 6.11.1 `msvc2022_64`)
- CMake 3.19+
- Ninja
- MSVC 2022 (Visual Studio 17/18) with 64-bit tools

### Steps

Open **x64 Native Tools Command Prompt** (or run `vcvars64.bat`), then:

```powershell
cd F:\globe
cmake -B build-msvc -S . -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/Qt/6.11.1/msvc2022_64
cmake --build build-msvc --target earth
```

Run:

```powershell
.\build-msvc\earth.exe
```

To run tests:

```powershell
cd build-msvc
ctest -C Release --output-on-failure
```

The app ships an editable `data/ip-services.json` next to `earth.exe` so you can change or reorder the IP geolocation services without recompiling.

## Acknowledgements

- Earth textures from [Solar System Scope](https://www.solarsystemscope.com/textures/)
- City data from [SimpleMaps World Cities](https://simplemaps.com/data/world-cities)
- Built with [Qt6](https://www.qt.io/) and OpenGL

## License

MIT License — see [LICENSE](LICENSE).

## Author

Created by HUANG Cheng.

Repository: https://github.com/huangcheng/solar-system
