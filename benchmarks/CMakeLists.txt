add_executable(benchmarks benchmarks.cpp)
common_compile_options(benchmarks)
target_link_libraries(benchmarks
    PRIVATE
        Boost::headers
)