# cm        - CLI that controls the runner (console app)
# cm_runner - resident key-mapping process (GUI subsystem: no console window)

# src/app.rc embeds an asInvoker manifest into both binaries. Without it, UAC
# installer-detection can treat the unmanifested exe as elevation-requiring,
# and Windows silently skips the HKCU\Run auto-start entry at logon.
enable_language(RC)

add_executable(cm
  ${CM_COMMON_DIR}/main.c
  src/process.c
  src/app.rc
)

add_executable(cm_runner
  src/runner.c
  src/app.rc
)

foreach(target cm cm_runner)
  target_include_directories(${target} PRIVATE src ${CM_COMMON_DIR})
  if(NOT MSVC)
    target_compile_options(${target} PRIVATE -Wall -Wextra -O2)
  endif()
endforeach()

# GUI subsystem replaces the old FreeConsole() hack: no console window is
# ever created for the runner.
set_target_properties(cm_runner PROPERTIES WIN32_EXECUTABLE TRUE)
if(MSVC)
  # MinGW links main() fine under -mwindows; MSVC needs the entry point named.
  target_link_options(cm_runner PRIVATE /ENTRY:mainCRTStartup)
endif()
