cmake_minimum_required(VERSION 3.13)
project(cm C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# The CLI dispatch (packages/common) is shared; each package provides the
# command table and the platform-specific handlers.
set(CM_COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/packages/common)

if(WIN32)
  add_subdirectory(packages/windows)
else()
  add_subdirectory(packages/linux)
endif()
