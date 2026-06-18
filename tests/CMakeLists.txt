find_package(Qt6 REQUIRED COMPONENTS Test)

function(globe_add_test name)
    add_executable(${name} ${name}.cpp)
    target_include_directories(${name} PRIVATE ${CMAKE_SOURCE_DIR}/src/globe)
    target_link_libraries(${name} PRIVATE globe PRIVATE Qt::Test)
    add_test(NAME ${name} COMMAND ${name})
endfunction()
