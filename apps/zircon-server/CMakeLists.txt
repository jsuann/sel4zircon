cmake_minimum_required(VERSION 3.7.2)

project(zircon-server C CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions -fno-unwind-tables")

file(GLOB deps
    src/*.c
    src/utils/*.c
    src/*.cxx
)

MakeCPIO(archive.o "$<TARGET_FILE:zircon-test:hello-world>")

add_executable(zircon-server EXCLUDE_FROM_ALL ${deps} archive.o)
target_include_directories(zircon-server PRIVATE "include")
target_link_libraries(zircon-server Configuration sel4 muslc sel4muslcsys sel4simple sel4vka sel4allocman sel4vspace sel4platsupport platsupport sel4utils cpio elf utils sel4simple-default sel4debug zircon)

DeclareRootserver(sel4test-driver)
