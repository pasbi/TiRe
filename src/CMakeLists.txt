qt_add_executable(tire main.cpp
        intervalmodel.cpp
        intervalmodel.h
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        interval.cpp
        interval.h
        datetimeeditor.cpp
        datetimeeditor.h
        datetimeeditor.ui
        leadingzerosspinbox.cpp
        leadingzerosspinbox.h
        projecteditor.cpp
        projecteditor.h
        projecteditor.ui
        json.cpp
        json.h
        exceptions.h
        periodsummary.cpp
        periodsummary.h
        periodsummary.ui
        period.cpp
        period.h
        fmt.h
        tableview.cpp
        tableview.h
        project.cpp
        project.h
        serialization.cpp
        serialization.h
        projectmodel.cpp
        projectmodel.h
        timesheet.cpp
        timesheet.h
        plan.cpp
        plan.h
        addremoveintervalcommand.cpp
        addremoveintervalcommand.h
        undostack.cpp
        undostack.h
        command.cpp
        command.h
)

target_link_libraries(tire PUBLIC Qt6::Core Qt6::Widgets spdlog::spdlog fmt::fmt)
target_compile_features(tire PUBLIC cxx_std_20)
target_include_directories(tire PUBLIC ${CMAKE_SOURCE_DIR}/src)
