cmake_minimum_required(VERSION 3.7.2)

project(libzircon C)

file(GLOB deps src/*.c)
list(SORT deps)

add_library(zircon STATIC EXCLUDE_FROM_ALL ${deps})

target_include_directories(zircon PUBLIC include)

target_link_libraries(zircon Configuration sel4 muslc elf utils sel4utils)
