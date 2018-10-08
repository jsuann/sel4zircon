cmake_minimum_required(VERSION 3.7.2)

project(zircon-test C)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -u __vsyscall_ptr -Wl,--section-start=.init=1000120")

file(GLOB deps
    src/*.c
    src/*.cxx
)

add_executable(sel4test-tests EXCLUDE_FROM_ALL ${deps})

target_link_libraries(Configuration sel4 elf muslc sel4muslcsys utils zircon zxutils)
