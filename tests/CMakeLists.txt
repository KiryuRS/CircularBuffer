message(STATUS "Tests enabled")

enable_testing()
find_package(GTest)

function(add_unit_test TEST_FILENAME_WITHOUT_CPP)
    add_executable(${TEST_FILENAME_WITHOUT_CPP} ${TEST_FILENAME_WITHOUT_CPP}.cpp)
    common_compile_options(${TEST_FILENAME_WITHOUT_CPP})
    target_include_directories(${TEST_FILENAME_WITHOUT_CPP} PRIVATE "${PROJECT_SOURCE_DIR}/src/")
    target_link_libraries(${TEST_FILENAME_WITHOUT_CPP}
        PRIVATE
            GTest::gtest_main
            GTest::gtest
            pthread
            -l:libgmock.a # TODO: Resolve without specifying this library
    )
    add_test(${TEST_FILENAME_WITHOUT_CPP} "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TEST_FILENAME_WITHOUT_CPP}")
endfunction()

add_unit_test(test_fifo)
add_unit_test(test_spmc)
