# Globe

一个 Windows 桌面地球仪小部件，支持真实昼夜、扁平地图模式，并正在扩展太阳系视图。

![应用图标](src/celestial/data/icon.png)

## 功能

- **3D 地球仪** — 以玩具速度旋转（最高 2880 倍速），带真实昼夜效果、夜晚城市灯光，以及可选网格。
- **扁平地图** — 等距圆柱世界地图，实时显示昼夜界线。
- **位置感知** — 选择家乡城市，通过系统 GPS/WiFi 或 IP 地理定位检测位置，并在地球仪/地图上显示家乡标记。
- **系统托盘** — 隐藏/显示、重置视图、以我为中心、设置、关于、退出。
- **设置** — 语言（English / 简体中文）、视图模式、夜晚模式、旋转速度、置顶、家乡位置。
- **关于对话框** — 版本、致谢、许可证与链接。

## 构建

### 环境要求

- Windows 10/11
- Qt 6.5+（已使用 Qt 6.11.1 `msvc2022_64` 测试）
- CMake 3.19+
- Ninja
- MSVC 2022（Visual Studio 17/18）64 位工具链

### 步骤

打开 **x64 本机工具命令提示符**（或运行 `vcvars64.bat`），然后：

```powershell
cd F:\globe
cmake -B build-msvc -S . -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/Qt/6.11.1/msvc2022_64
cmake --build build-msvc --target earth
```

运行：

```powershell
.\build-msvc\earth.exe
```

运行测试：

```powershell
cd build-msvc
ctest -C Release --output-on-failure
```

程序会在 `earth.exe` 旁边附带可编辑的 `data/ip-services.json`，你可以在不重新编译的情况下更改或调整 IP 地理定位服务。

## 致谢

- 地球纹理来自 [Solar System Scope](https://www.solarsystemscope.com/textures/)
- 城市数据来自 [SimpleMaps World Cities](https://simplemaps.com/data/world-cities)
- 使用 [Qt6](https://www.qt.io/) 与 OpenGL 构建

## 许可证

MIT License — 详见 [LICENSE](LICENSE)。

## 作者

Created by HUANG Cheng.

仓库地址：https://github.com/huangcheng/solar-system
