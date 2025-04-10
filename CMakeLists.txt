cmake_minimum_required(VERSION 3.18)

project(tire)

find_package(Qt6 6.4 REQUIRED Core Widgets Network)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_subdirectory(src)
if (BUILD_TESTING)
    add_subdirectory(googletest)
    add_subdirectory(benchmarks)
    include(CTest)
    add_subdirectory(test/unit)
endif ()
