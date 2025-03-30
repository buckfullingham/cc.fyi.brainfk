find_package(readline REQUIRED)
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

add_library(llvm-libs INTERFACE)
target_include_directories(llvm-libs SYSTEM INTERFACE ${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
target_compile_definitions(llvm-libs INTERFACE ${LLVM_DEFINITIONS_LIST})
llvm_map_components_to_libnames(llvm_libs
        mcjit
        "${CMAKE_HOST_SYSTEM_PROCESSOR}asmparser"
        "${CMAKE_HOST_SYSTEM_PROCESSOR}codegen"
        "${CMAKE_HOST_SYSTEM_PROCESSOR}disassembler"
)
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
