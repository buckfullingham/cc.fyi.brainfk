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

add_test(
        NAME integration_test_handrolled
        COMMAND
        sh -c "${CMAKE_BINARY_DIR}/src/main/ccbf -m handrolled ${CMAKE_SOURCE_DIR}/src/test/resources/mandelbrot.bf | diff ${CMAKE_SOURCE_DIR}/src/test/resources/mandelbrot.txt -"
)

add_test(
        NAME integration_test_llvm_optimised
        COMMAND
        sh -c "${CMAKE_BINARY_DIR}/src/main/ccbf -m llvm -o ${CMAKE_SOURCE_DIR}/src/test/resources/mandelbrot.bf | diff ${CMAKE_SOURCE_DIR}/src/test/resources/mandelbrot.txt -"
)

add_test(
        NAME integration_test_llvm_unoptimised
        COMMAND
        sh -c "${CMAKE_BINARY_DIR}/src/main/ccbf -m llvm -O ${CMAKE_SOURCE_DIR}/src/test/resources/mandelbrot.bf | diff ${CMAKE_SOURCE_DIR}/src/test/resources/mandelbrot.txt -"
)
