cmake_minimum_required(VERSION 3.19)
project(globe LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets Positioning LinguistTools Test)

qt_standard_project_setup()

# Core library: holds all globe sources (AUTOMOC-enabled) so both the app and
# the tests can link the same code. (An executable target cannot be linked into
# another target, which is why this is a STATIC library.)
qt_add_library(globe_lib STATIC
    src/globe/GlobeWindow.h
    src/globe/GlobeWindow.cpp
    src/globe/SystemTray.h
    src/globe/SystemTray.cpp
    src/globe/PlatformWindow.h
    src/globe/PlatformWindow.cpp
    src/globe/ConfigManager.h
    src/globe/ConfigManager.cpp
    src/globe/LocationProvider.h
    src/globe/LocationProvider.cpp
    src/globe/SettingsDialog.h
    src/globe/SettingsDialog.cpp
    src/globe/FullscreenWatcher.h
    src/globe/FullscreenWatcher.cpp
    src/globe/render/SunModel.h
    src/globe/render/SunModel.cpp
    src/globe/render/CameraController.h
    src/globe/render/CameraController.cpp
    src/globe/render/AssetManager.h
    src/globe/render/AssetManager.cpp
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
)

target_link_libraries(globe_lib PUBLIC
    Qt::Core Qt::Gui Qt::Widgets Qt::OpenGL Qt::OpenGLWidgets Qt::Positioning
)

if(WIN32)
    target_link_libraries(globe_lib PUBLIC dwmapi user32)
endif()

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
