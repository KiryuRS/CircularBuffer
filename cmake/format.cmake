function(enable_formatting)
    # all source files
    file(GLOB_RECURSE ALL_SOURCE_FILES "src/*.cpp" "src/*.hpp" "src/*.hxx")
    # all test files
    file(GLOB_RECURSE ALL_TEST_FILES "tests/*.cpp" "tests/*.hpp" "tests/*.hxx")
    add_custom_target(clang-format
            COMMAND /usr/bin/clang-format-15 --style=file:"${PROJECT_SOURCE_DIR}/.clang-format" -i ${ALL_SOURCE_FILES} ${ALL_TEST_FILES}
    )
endfunction()
