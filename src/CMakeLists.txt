qt_add_executable(tire main.cpp
        datetimeeditor.cpp
        datetimeeditor.h
        datetimeeditor.ui
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
        leadingzerosspinbox.cpp
        leadingzerosspinbox.h
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        period.cpp
        period.h
        plan.cpp
        plan.h
        project.cpp
        project.h
        projecteditor.cpp
        projecteditor.h
        projecteditor.ui
        projectmodel.cpp
        projectmodel.h
        serialization.cpp
        serialization.h
        tableview.cpp
        tableview.h
        timesheet.cpp
        timesheet.h
)

target_link_libraries(tire PUBLIC Qt6::Core Qt6::Widgets spdlog::spdlog fmt::fmt)
target_compile_features(tire PUBLIC cxx_std_20)
target_include_directories(tire PUBLIC ${CMAKE_SOURCE_DIR}/src)

add_subdirectory(commands)
add_subdirectory(views)
