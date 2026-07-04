# cm        - CLI that controls the runner
# cm_runner - resident key-mapping daemon (evdev/uinput; requires root)
#
# Built via the top-level CMakeLists.txt, which provides CM_COMMON_DIR.

add_executable(cm
  ${CM_COMMON_DIR}/main.c
  src/process.c
)

add_executable(cm_runner
  src/runner.c
)

foreach(target cm cm_runner)
  target_include_directories(${target} PRIVATE src ${CM_COMMON_DIR})
  target_compile_options(${target} PRIVATE -Wall -Wextra -O2)
  target_compile_definitions(${target} PRIVATE _GNU_SOURCE)
endforeach()
