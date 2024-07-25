cmake_minimum_required(VERSION 3.18)

project(tire)

find_package(Qt6 6.4 REQUIRED Core Widgets)
find_package(nlohmann_json REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_subdirectory(src)
