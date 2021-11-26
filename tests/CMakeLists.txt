set(all_sources
        tests/generator_tests.cpp
        tests/single_task_tests.cpp tests/helpers.hpp)

add_executable(
        tests
        ${all_sources}
)


target_link_libraries(tests PUBLIC gtest_main)

include(GoogleTest)
gtest_discover_tests(tests)