cmake_minimum_required(VERSION 3.19)
project(globe VERSION 1.0.0 LANGUAGES CXX)

# Version metadata injected into the app for the About dialog.
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GLOBE_GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT GLOBE_GIT_HASH)
    set(GLOBE_GIT_HASH "unknown")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets Positioning Network LinguistTools Test)

qt_standard_project_setup()

# Generic, app-agnostic celestial mechanics / camera / asset loading shared by
# every body widget (earth, sun, mars, ...). Kept as a separate STATIC library
# so each app links only what it needs while reusing the same code.
qt_add_library(celestial STATIC
    src/celestial/AboutDialog.h
    src/celestial/AboutDialog.cpp
    src/celestial/AssetManager.h
    src/celestial/AssetManager.cpp
    src/celestial/CameraController.h
    src/celestial/CameraController.cpp
    src/celestial/SunModel.h
    src/celestial/SunModel.cpp
    src/celestial/ConfigManager.h
    src/celestial/ConfigManager.cpp
    src/celestial/SettingsDialog.h
    src/celestial/SettingsDialog.cpp
    src/celestial/SystemTray.h
    src/celestial/SystemTray.cpp
    src/celestial/FullscreenWatcher.h
    src/celestial/FullscreenWatcher.cpp
    src/celestial/PlatformWindow.h
    src/celestial/PlatformWindow.cpp
    src/celestial/LocationProvider.h
    src/celestial/LocationProvider.cpp
    src/celestial/BodyConfig.h
    src/celestial/BodyConfig.cpp
    src/celestial/TimeController.h
    src/celestial/TimeController.cpp
    src/celestial/CelestialBody.h
    src/celestial/CelestialBody.cpp
    src/celestial/CelestialWidget.h
    src/celestial/CelestialWidget.cpp
    src/celestial/TerminatorModel.h
    src/celestial/TerminatorModel.cpp
    src/celestial/CityDatabase.h
    src/celestial/CityDatabase.cpp
    src/celestial/cities.qrc
    src/celestial/IpLocator.h
    src/celestial/IpLocator.cpp
)

target_include_directories(celestial PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/celestial
)

# AUTORCC is not turned on by qt_standard_project_setup() in this Qt; enable it
# explicitly so the bundled cities.qrc is rcc-compiled into the static lib
# (resolves at runtime as :/data/cities.json).
set_target_properties(celestial PROPERTIES AUTORCC ON)

target_link_libraries(celestial PUBLIC
    Qt::Core Qt::Gui Qt::Widgets Qt::OpenGL Qt::OpenGLWidgets Qt::Positioning Qt::Network
)

target_compile_definitions(celestial PRIVATE
    GLOBE_VERSION="${PROJECT_VERSION}"
    GLOBE_GIT_HASH="${GLOBE_GIT_HASH}"
)

if(WIN32)
    target_link_libraries(celestial PUBLIC dwmapi user32)
endif()

qt_add_executable(earth
    WIN32 MACOSX_BUNDLE
    src/apps/earth/main.cpp
)
target_link_libraries(earth PRIVATE celestial)

qt_add_translations(TARGETS earth TS_FILES globe_zh_CN.ts)

# Ship the editable IP service list next to the built earth.exe so it can be
# tuned post-ship without a recompile. IpLocator reads appDir/data/ip-services.json
# and falls back to a compiled-in default if the file is missing/corrupt.
add_custom_command(TARGET earth POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:earth>/data"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/src/celestial/data/ip-services.json"
        "$<TARGET_FILE_DIR:earth>/data/ip-services.json")

install(TARGETS earth
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES src/celestial/data/ip-services.json DESTINATION data)

qt_generate_deploy_app_script(TARGET earth OUTPUT_SCRIPT deploy_script NO_UNSUPPORTED_PLATFORM_ERROR)
install(SCRIPT ${deploy_script})

# Tests
enable_testing()
add_subdirectory(tests)
