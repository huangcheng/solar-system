find_package(Qt6 REQUIRED COMPONENTS Test)

function(globe_add_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE globe_lib Qt::Test)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

globe_add_test(test_configmanager)

globe_add_test(test_sunmodel)

globe_add_test(test_cameracontroller)
globe_add_test(test_sunview)

globe_add_test(test_locationprovider)

# Offscreen render harness (not a ctest — needs a GL context). Built on demand
# to capture a globe frame to PNG for visual inspection of shader changes.
add_executable(render_shot render_shot.cpp)
target_link_libraries(render_shot PRIVATE globe_lib Qt::Gui)
