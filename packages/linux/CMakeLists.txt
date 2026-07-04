cmake_minimum_required(VERSION 3.10)
project(CapsLockMouseLinux C)

set(CMAKE_C_STANDARD 11)
add_definitions(-D_GNU_SOURCE)
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wno-unused-result)
    # add_compile_options(-Wall -Wextra -O2)
endif()

include_directories(
    src
    src/constants
    src/utils
)

set(COMMON_SOURCES
    src/constants/app_constants.c   
    src/constants/result_constants.c
    src/process.c
)

add_executable(cm src/main.c ${COMMON_SOURCES})
add_executable(cm_runner src/runner.c ${COMMON_SOURCES})

