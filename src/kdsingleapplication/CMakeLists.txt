target_sources(tire-impl PRIVATE
        kdsingleapplication.cpp
        kdsingleapplication.h
        kdsingleapplication_localsocket.cpp
        kdsingleapplication_localsocket_p.h
)

# These sources are compiled straight into tire-impl rather than into a library of their
# own, so say so: kdsingleapplication_lib.h otherwise falls through to Q_DECL_IMPORT. Under
# MSVC that is __declspec(dllimport), which makes moc's definition of staticMetaObject
# illegal; under GCC and Clang it is only a visibility attribute, which is why this went
# unnoticed on Linux. PUBLIC because src/main.cpp includes the header from the tire target,
# and a mismatch there would be a link error against __imp_ symbols rather than a warning.
target_compile_definitions(tire-impl PUBLIC KDSINGLEAPPLICATION_STATIC_BUILD)
