find_package(readline REQUIRED)

add_library(brainfk-objects OBJECT
        repl.cpp
        readline.cpp
)

target_link_libraries(brainfk-objects PUBLIC
        readline::readline
)

add_executable(ccbf
        ccbf.cpp
)

target_link_libraries(ccbf PRIVATE
        brainfk-objects
)
