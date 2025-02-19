find_package(Catch2 REQUIRED)
find_package(fakeit REQUIRED)

include(CTest)
include(Catch)

add_library(brainfk-tests INTERFACE)

target_include_directories(brainfk-tests INTERFACE
        ../main
)

target_link_libraries(brainfk-tests INTERFACE
)

add_executable(tests
        tests.cpp
)

target_link_libraries(tests PRIVATE
        Catch2::Catch2WithMain
        brainfk-tests
        brainfk-objects
        fakeit::fakeit
)

catch_discover_tests(tests EXTRA_ARGS "--rng-seed=${PRNG_SEED}")
