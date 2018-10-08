cmake_minimum_required(VERSION 3.7.2)

include(${KERNEL_HELPERS_PATH})

add_subdirectory(apps/hello-world)
add_subdirectory(apps/zircon-server)
add_subdirectory(apps/zircon-test)

add_subdirectory(libzircon)
add_subdirectory(libzxutils)

GenerateSimulateScript()

# Set config?
