find_package(Catch2 REQUIRED)

include(CTest)
include(Catch)

add_executable(tests
        tests.cpp
)

target_link_libraries(tests PRIVATE
        brainfk-objects
)

catch_discover_tests(tests EXTRA_ARGS "--rng-seed=${PRNG_SEED}")
