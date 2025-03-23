find_package(readline REQUIRED)
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

add_library(brainfk-objects OBJECT
        repl.cpp
        readline.cpp
)

target_link_libraries(brainfk-objects PUBLIC
        readline::readline
)

target_include_directories(brainfk-objects SYSTEM PUBLIC
        ${LLVM_INCLUDE_DIRS}
)

target_compile_definitions(brainfk-objects PUBLIC
        ${LLVM_DEFINITIONS}
)

llvm_map_components_to_libnames(LLVM_LIBS core)

target_link_libraries(brainfk-objects PUBLIC
        ${LLVM_LIBS}
)

add_executable(ccbf
        ccbf.cpp
)

target_link_libraries(ccbf PRIVATE
        brainfk-objects
)

add_executable(llvm
        llvm.cpp)

target_compile_options(llvm PRIVATE
        -Wno-unused-variable
)
target_link_libraries(llvm PRIVATE
        ${LLVM_LIBS}
)

target_compile_definitions(llvm PRIVATE
        ${LLVM_DEFINITIONS}
)

target_include_directories(llvm SYSTEM PRIVATE
        ${LLVM_INCLUDE_DIRS}
)
