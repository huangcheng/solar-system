cmake_minimum_required(VERSION 3.19)
project(globe LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets LinguistTools Test)

qt_standard_project_setup()

qt_add_executable(globe
    WIN32 MACOSX_BUNDLE
    main.cpp
    src/globe/GlobeWindow.h
    src/globe/GlobeWindow.cpp
)

target_include_directories(globe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(globe PRIVATE
    Qt::Core Qt::Gui Qt::Widgets Qt::OpenGL Qt::OpenGLWidgets
)

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
