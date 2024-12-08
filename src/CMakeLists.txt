qt_add_executable(tire main.cpp)
add_library(tire-impl STATIC)
target_sources(tire-impl PRIVATE
        application.cpp
        application.h
        datetimeeditor.cpp
        datetimeeditor.h
        datetimeeditor.ui
        enum.h
        enumcombobox.h
        exceptions.h
        fmt.h
        ganttview.cpp
        ganttview.h
        interval.cpp
        interval.h
        intervalmodel.cpp
        intervalmodel.h
        json.cpp
        json.h
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        period.cpp
        period.h
        periodedit.cpp
        periodedit.h
        periodedit.ui
        plan.cpp
        plan.h
        plantableview.cpp
        plantableview.h
        project.cpp
        project.h
        projectmodel.cpp
        projectmodel.h
        serialization.cpp
        serialization.h
        tableview.cpp
        tableview.h
        timesheet.cpp
        timesheet.h
        colorutil.cpp
        colorutil.h
        timeedit.cpp
        timeedit.h
        timerangeeditor.cpp
        timerangeeditor.h
        timerangeeditor.ui
        datetimeselector.cpp
        datetimeselector.h
        splitpointeditor.cpp
        splitpointeditor.h
        splitpointeditor.ui
)

target_link_libraries(tire-impl PUBLIC Qt6::Core Qt6::Widgets spdlog::spdlog fmt::fmt nlohmann_json::nlohmann_json)
target_link_libraries(tire PRIVATE tire-impl)
target_compile_features(tire PUBLIC cxx_std_20)
target_compile_features(tire-impl PUBLIC cxx_std_20)
target_include_directories(tire-impl PUBLIC ${CMAKE_SOURCE_DIR}/src)

add_subdirectory(commands)
add_subdirectory(views)
