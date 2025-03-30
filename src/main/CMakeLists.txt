find_package(readline REQUIRED)
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

add_library(llvm-libs INTERFACE)
target_include_directories(llvm-libs SYSTEM INTERFACE ${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
target_compile_definitions(llvm-libs INTERFACE ${LLVM_DEFINITIONS_LIST})
execute_process(COMMAND llvm-config-19 --libs all OUTPUT_VARIABLE llvm_libs_raw)
string(STRIP "${llvm_libs_raw}" llvm_libs)
target_link_libraries(llvm-libs INTERFACE ${llvm_libs})

add_library(brainfk-objects OBJECT
        llvm.cpp
        readline.cpp
        repl.cpp
)

target_link_libraries(brainfk-objects PUBLIC
        readline::readline
        llvm-libs
)

add_executable(ccbf
        ccbf.cpp
)

target_link_libraries(ccbf PRIVATE
        brainfk-objects
)
