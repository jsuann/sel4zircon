cmake_minimum_required(VERSION 3.7.2)

project(libzxutils C)

file(GLOB deps src/*.c)
list(SORT deps)

add_library(zxutils STATIC EXCLUDE_FROM_ALL ${deps})

target_include_directories(zxutils PUBLIC include)

target_link_libraries(zxutils Configuration muslc zircon)
