cmake_minimum_required(VERSION 3.7.2)

project(hello-world C)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--section-start=.text=1000120")

# disable crt? CRTObjFiles FinObjFiles ?

file(GLOB deps
    src/*.c
    src/*.S
)

target_link_libraries(Configuration sel4 zircon zxutils)

add_executable(hello-world EXCLUDE_FROM_ALL ${deps})
