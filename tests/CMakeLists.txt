find_package(Qt6 REQUIRED COMPONENTS Test)

function(globe_add_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE celestial Qt::Test)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

globe_add_test(test_configmanager)

globe_add_test(test_sunmodel)

globe_add_test(test_cameracontroller)
globe_add_test(test_sunview)

globe_add_test(test_locationprovider)

globe_add_test(test_bodyconfig)

globe_add_test(test_terminator)

globe_add_test(test_citydatabase)

globe_add_test(test_iplocator)
