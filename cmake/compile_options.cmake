function(common_compile_options TARGET_NAME)

    # properties
    set_target_properties(${TARGET_NAME}
        PROPERTIES
            LINKER_LANGUAGE         CXX
            CXX_STANDARD            23
            CXX_STANDARD_REQUIRED   ON
            CXX_EXTENSIONS          OFF
    )

    # Use ccache if available
    if (CCACHE_FOUND)
        set_target_properties(${TARGET_NAME}
                PROPERTIES
                CXX_COMPILER_LAUNCHER ccache
        )
    endif()

    # compiler flags
    target_compile_options(${TARGET_NAME}
        PRIVATE
            -Wall
            -Wextra
            -Werror
            -Wcast-align
            -Wcast-qual
            -Wuninitialized
            -Wmissing-include-dirs
            -Wuseless-cast
            -Wstring-compare
            -Wconversion
            -Wno-interference-size

            -fexceptions
            -fstrict-aliasing
            -fdiagnostics-color=always
    )

    # optimization flags
    target_compile_options(${TARGET_NAME}
        PRIVATE
            $<$<CONFIG:Debug>:-O0 -ggdb>
            $<$<CONFIG:Release>:-O3>
            $<$<CONFIG:RelWithDebInfo>:-O2 -g>
    )

    # Additional arguments
    target_compile_options(${TARGET_NAME}
        PRIVATE
            ${ARGN}
    )

endfunction()
