include(GoogleTest)

macro(package_add_test SOURCE_FILE)
    string(REGEX REPLACE "\.[^.]*$" "" TESTNAME "${SOURCE_FILE}")
    add_executable(${TESTNAME} ${SOURCE_FILE})
    target_link_libraries(${TESTNAME} PRIVATE GTest::gtest_main tire-impl)
    gtest_discover_tests(${TESTNAME})
endmacro()

package_add_test(colortest.cpp)
package_add_test(periodtest.cpp)
package_add_test(plantest.cpp)
