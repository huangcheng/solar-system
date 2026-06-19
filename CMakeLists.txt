cmake_minimum_required(VERSION 3.19)
project(globe LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets Positioning LinguistTools Test)

qt_standard_project_setup()

# Generic, app-agnostic celestial mechanics / camera / asset loading shared by
# every body widget (earth, sun, mars, ...). Kept as a separate STATIC library
# so each app links only what it needs while reusing the same code.
qt_add_library(celestial STATIC
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
)

target_include_directories(celestial PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/celestial
)

target_link_libraries(celestial PUBLIC
    Qt::Core Qt::Gui Qt::Widgets Qt::Positioning
)

if(WIN32)
    target_link_libraries(celestial PUBLIC dwmapi user32)
endif()

# Core library: holds all globe sources (AUTOMOC-enabled) so both the app and
# the tests can link the same code. (An executable target cannot be linked into
# another target, which is why this is a STATIC library.)
qt_add_library(globe_lib STATIC
    src/globe/GlobeWindow.h
    src/globe/GlobeWindow.cpp
    src/globe/render/GlobeRenderer.h
    src/globe/render/GlobeRenderer.cpp
    src/globe/render/GlobeView.h
    src/globe/render/GlobeView.cpp
    src/globe/render/TimeController.h
    src/globe/render/TimeController.cpp
)

target_include_directories(globe_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/globe
    ${CMAKE_CURRENT_SOURCE_DIR}/src/globe/render
    ${CMAKE_CURRENT_SOURCE_DIR}/src/celestial
)

target_link_libraries(globe_lib PUBLIC
    celestial
    Qt::Widgets Qt::OpenGL Qt::OpenGLWidgets
)

qt_add_executable(globe
    WIN32 MACOSX_BUNDLE
    main.cpp
)
target_link_libraries(globe PRIVATE globe_lib)

qt_add_translations(TARGETS globe TS_FILES globe_zh_CN.ts)

install(TARGETS globe
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_app_script(TARGET globe OUTPUT_SCRIPT deploy_script NO_UNSUPPORTED_PLATFORM_ERROR)
install(SCRIPT ${deploy_script})

# Tests
enable_testing()
add_subdirectory(tests)
