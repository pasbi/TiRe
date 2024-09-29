
include(GoogleTest)
add_executable(tests
        test.cpp
)
target_link_libraries(tests PRIVATE GTest::gtest_main)
gtest_discover_tests(tests)
