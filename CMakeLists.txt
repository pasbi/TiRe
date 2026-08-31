# 3.24 for FetchContent's FIND_PACKAGE_ARGS.
cmake_minimum_required(VERSION 3.24)

project(tire)

find_package(Qt6 6.4 REQUIRED Core Widgets Network Sql)

# src/fmt.h specializes fmt::formatter directly, so spdlog has to share that fmt rather
# than compile its bundled copy into its own namespace.
set(SPDLOG_FMT_EXTERNAL ON)

# FIND_PACKAGE_ARGS makes these prefer an installed copy and only build from source when
# there is none: distro packages on a dev machine, a source build on a CI runner. Both
# compile in seconds, which is why they need no package manager of their own.
include(FetchContent)
FetchContent_Declare(fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 12.2.0
        GIT_SHALLOW ON
        FIND_PACKAGE_ARGS 10.1
)
FetchContent_Declare(spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.17.0
        GIT_SHALLOW ON
        FIND_PACKAGE_ARGS 1.15
)
# fmt first: spdlog only skips its own find_package(fmt) when the target already exists.
FetchContent_MakeAvailable(fmt spdlog)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

include(CTest)

add_subdirectory(src)
if (BUILD_TESTING)
    add_subdirectory(googletest)
    include(CTest)
    add_subdirectory(benchmarks)
    add_subdirectory(test/unit)
endif ()
