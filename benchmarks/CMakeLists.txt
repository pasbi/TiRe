add_executable(add-interval-benchmark addintervalbenchmark.cpp)
add_library(tire-benchmark-utils STATIC
        createlargeproject.cpp
        createlargeproject.h
)
target_compile_features(add-interval-benchmark PUBLIC cxx_std_20)
target_compile_features(tire-benchmark-utils PUBLIC cxx_std_20)
target_link_libraries(tire-benchmark-utils PUBLIC tire-impl)
target_link_libraries(add-interval-benchmark PUBLIC tire-impl tire-benchmark-utils)
