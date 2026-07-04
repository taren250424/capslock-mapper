#ifndef APP_H
#define APP_H

#define APP_NAME "cm"
#define APP_VERSION "v2.0.0"

// File name of the resident mapper process, expected to sit in the same
// directory as cm.exe.
#define RUNNER_EXE "cm_runner.exe"

// Named kernel objects shared between cm.exe and cm_runner.exe.
// The mutex marks "a runner is alive"; the event asks it to shut down.
#define RUNNER_MUTEX_NAME "Global\\CM_RUNNER_MUTEX"
#define RUNNER_STOP_EVENT_NAME "Global\\CM_RUNNER_STOP"

// HKCU key that holds per-user auto-start entries.
#define STARTUP_REG_PATH "Software\\Microsoft\\Windows\\CurrentVersion\\Run"

#endif
