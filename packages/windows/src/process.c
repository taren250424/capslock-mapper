// Command handlers for the Windows CLI. The dispatch loop lives in
// packages/common/main.c; this file supplies the command table and the
// platform-specific work.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "app.h"
#include "cli.h"

// Older MinGW headers don't define this console-mode flag.
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Directory of cm.exe, including the trailing backslash. Returns 1 on success.
static int get_exe_dir(char* dir, DWORD size) {
  DWORD len = GetModuleFileNameA(NULL, dir, size);
  if (len == 0 || len >= size) {
    fprintf(stderr, "Failed to get the executable path.\n");
    return 0;
  }
  char* lastSep = strrchr(dir, '\\');
  if (!lastSep) {
    fprintf(stderr, "Unexpected executable path: %s\n", dir);
    return 0;
  }
  lastSep[1] = '\0';
  return 1;
}

// The runner holds this mutex for its whole lifetime, so its mere existence
// answers "is the mapper running?".
static int runner_is_running(void) {
  HANDLE mutex = OpenMutexA(SYNCHRONIZE, FALSE, RUNNER_MUTEX_NAME);
  if (!mutex) {
    return 0;
  }
  CloseHandle(mutex);
  return 1;
}

// Reads HKCU\Environment's Path value into a malloc'd buffer with `extra`
// spare bytes for appending. RRF_NOEXPAND keeps REG_EXPAND_SZ values raw so
// we can write them back without destroying %VAR% references; the original
// value type is reported through `type` for that purpose.
// On success the caller owns the buffer and must RegCloseKey(*key).
static char* read_user_path(HKEY* key, DWORD extra, DWORD* type) {
  if (RegOpenKeyExA(HKEY_CURRENT_USER, "Environment", 0, KEY_READ | KEY_WRITE, key) !=
      ERROR_SUCCESS) {
    fprintf(stderr, "Failed to open HKCU\\Environment.\n");
    return NULL;
  }

  DWORD flags = RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND;
  DWORD size = 0;
  if (RegGetValueA(*key, NULL, "Path", flags, type, NULL, &size) != ERROR_SUCCESS) {
    fprintf(stderr, "Failed to query the Path environment variable.\n");
    RegCloseKey(*key);
    return NULL;
  }

  char* env = malloc(size + extra);
  if (!env) {
    fprintf(stderr, "Memory allocation failed.\n");
    RegCloseKey(*key);
    return NULL;
  }

  if (RegGetValueA(*key, NULL, "Path", flags, NULL, env, &size) != ERROR_SUCCESS) {
    fprintf(stderr, "Failed to read the Path environment variable.\n");
    free(env);
    RegCloseKey(*key);
    return NULL;
  }

  return env;
}

// Writes the Path value back with its original type and tells running apps
// (e.g. new terminals) that the environment changed.
static int write_user_path(HKEY key, const char* env, DWORD type) {
  if (RegSetValueExA(key, "Path", 0, type, (const BYTE*)env, (DWORD)strlen(env) + 1) !=
      ERROR_SUCCESS) {
    fprintf(stderr, "Failed to set the Path environment variable.\n");
    return 0;
  }
  SendMessageTimeoutA(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)"Environment",
                      SMTO_ABORTIFHUNG, 5000, NULL);
  return 1;
}

// Finds `dir` inside a Path value, matching whole entries only: the hit must
// be preceded by start-of-string or ';' and followed by ';' or end-of-string.
// A plain strstr would also match inside longer sibling paths.
static char* find_path_entry(char* env, const char* dir) {
  size_t dirLen = strlen(dir);
  for (char* p = strstr(env, dir); p; p = strstr(p + 1, dir)) {
    int entryStart = (p == env) || (p[-1] == ';');
    char after = p[dirLen];
    if (entryStart && (after == ';' || after == '\0')) {
      return p;
    }
  }
  return NULL;
}

// Returns 1 if the Run key currently holds an APP_NAME value.
static int startup_is_registered(void) {
  HKEY key;
  if (RegOpenKeyExA(HKEY_CURRENT_USER, STARTUP_REG_PATH, 0, KEY_QUERY_VALUE, &key) !=
      ERROR_SUCCESS) {
    return 0;
  }
  LONG result = RegQueryValueExA(key, APP_NAME, NULL, NULL, NULL, NULL);
  RegCloseKey(key);
  return result == ERROR_SUCCESS;
}

// ---------------------------------------------------------------------------
// on / off / status
// ---------------------------------------------------------------------------

static int on_runner(void) {
  if (runner_is_running()) {
    printf("Program is already running.\n");
    return EXIT_SUCCESS;
  }

  char path[MAX_PATH];
  if (!get_exe_dir(path, sizeof(path))) {
    return EXIT_FAILURE;
  }
  if (strlen(path) + strlen(RUNNER_EXE) + 1 > sizeof(path)) {
    fprintf(stderr, "Executable path is too long.\n");
    return EXIT_FAILURE;
  }
  strcat(path, RUNNER_EXE);

  // CreateProcess instead of system("start ..."): no cmd.exe quoting issues,
  // and the runner (a GUI-subsystem binary) detaches on its own.
  STARTUPINFOA si = {0};
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi = {0};
  if (!CreateProcessA(path, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
    fprintf(stderr, "Failed to start %s (error %lu).\n", RUNNER_EXE, GetLastError());
    return EXIT_FAILURE;
  }
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);

  printf("Program has started.\n");
  return EXIT_SUCCESS;
}

static int off_runner(void) {
  if (!runner_is_running()) {
    printf("Program was not running.\n");
    return EXIT_SUCCESS;
  }

  // Signal the stop event instead of taskkill /F, so the runner gets to
  // remove its keyboard hook and release its handles.
  HANDLE stop = OpenEventA(EVENT_MODIFY_STATE, FALSE, RUNNER_STOP_EVENT_NAME);
  if (!stop) {
    fprintf(stderr, "Failed to reach the runner (error %lu).\n", GetLastError());
    return EXIT_FAILURE;
  }
  SetEvent(stop);
  CloseHandle(stop);

  // The runner exits within a few milliseconds; poll briefly to confirm.
  for (int i = 0; i < 20; i++) {
    if (!runner_is_running()) {
      printf("Program has stopped.\n");
      return EXIT_SUCCESS;
    }
    Sleep(100);
  }

  fprintf(stderr, "Runner did not stop in time.\n");
  return EXIT_FAILURE;
}

static int show_status(void) {
  // Enable ANSI colors on this console.
  HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD mode = 0;
  if (out != INVALID_HANDLE_VALUE && GetConsoleMode(out, &mode)) {
    SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
  }

  int running = runner_is_running();
  int registered = startup_is_registered();

  // PATH check: is the directory containing cm.exe a whole entry of the
  // user's Path value?
  int envFound = 0;
  char dir[MAX_PATH];
  if (get_exe_dir(dir, sizeof(dir))) {
    HKEY key;
    DWORD type;
    char* env = read_user_path(&key, 1, &type);
    if (env) {
      envFound = find_path_entry(env, dir) != NULL;
      free(env);
      RegCloseKey(key);
    }
  }

  printf("[Running status]    %s\n",
         running ? "\033[0;32mRunning.\033[0m" : "\033[0;31mNot running.\033[0m");
  printf("[Env status]        %s\n",
         envFound ? "\033[0;32mFound.\033[0m" : "\033[0;31mNot found.\033[0m");
  printf("[Startup status]    %s\n",
         registered ? "\033[0;32mRegistered.\033[0m" : "\033[0;31mNot registered.\033[0m");
  return EXIT_SUCCESS;
}

// ---------------------------------------------------------------------------
// env --add / --remove
// ---------------------------------------------------------------------------

static int add_env(void) {
  char dir[MAX_PATH];
  if (!get_exe_dir(dir, sizeof(dir))) {
    return EXIT_FAILURE;
  }

  HKEY key;
  DWORD type;
  // Reserve room for ";" + dir.
  char* env = read_user_path(&key, (DWORD)strlen(dir) + 2, &type);
  if (!env) {
    return EXIT_FAILURE;
  }

  int result = EXIT_SUCCESS;
  if (find_path_entry(env, dir)) {
    printf("The environment path already includes this executable.\n");
  } else {
    size_t len = strlen(env);
    if (len > 0 && env[len - 1] != ';') {
      strcat(env, ";");
    }
    strcat(env, dir);

    if (write_user_path(key, env, type)) {
      printf("Environment path has been updated successfully.\n");
    } else {
      result = EXIT_FAILURE;
    }
  }

  free(env);
  RegCloseKey(key);
  return result;
}

static int remove_env(void) {
  char dir[MAX_PATH];
  if (!get_exe_dir(dir, sizeof(dir))) {
    return EXIT_FAILURE;
  }

  HKEY key;
  DWORD type;
  char* env = read_user_path(&key, 1, &type);
  if (!env) {
    return EXIT_FAILURE;
  }

  int result = EXIT_SUCCESS;
  char* entry = find_path_entry(env, dir);
  if (!entry) {
    printf("The specified path is not found in the environment variable.\n");
  } else {
    // Cut the entry plus one adjacent separator: the trailing ';' if the
    // entry is not last, otherwise the leading one (if any).
    size_t cutLen = strlen(dir);
    if (entry[cutLen] == ';') {
      cutLen++;
    } else if (entry > env && entry[-1] == ';') {
      entry--;
      cutLen++;
    }
    memmove(entry, entry + cutLen, strlen(entry + cutLen) + 1);

    if (write_user_path(key, env, type)) {
      printf("Environment path has been removed successfully.\n");
    } else {
      result = EXIT_FAILURE;
    }
  }

  free(env);
  RegCloseKey(key);
  return result;
}

// ---------------------------------------------------------------------------
// startup --add / --remove
// ---------------------------------------------------------------------------

static int add_startup(void) {
  if (startup_is_registered()) {
    printf("Already registered.\n");
    return EXIT_SUCCESS;
  }

  char dir[MAX_PATH];
  if (!get_exe_dir(dir, sizeof(dir))) {
    return EXIT_FAILURE;
  }

  // The Run key value is parsed as a command line, so quote the path in case
  // the install directory contains spaces.
  char command[MAX_PATH + 2 + sizeof(RUNNER_EXE)];
  snprintf(command, sizeof(command), "\"%s%s\"", dir, RUNNER_EXE);

  HKEY key;
  if (RegOpenKeyExA(HKEY_CURRENT_USER, STARTUP_REG_PATH, 0, KEY_SET_VALUE, &key) !=
      ERROR_SUCCESS) {
    fprintf(stderr, "Failed to open the startup registry key.\n");
    return EXIT_FAILURE;
  }
  LONG result =
      RegSetValueExA(key, APP_NAME, 0, REG_SZ, (const BYTE*)command, (DWORD)strlen(command) + 1);
  RegCloseKey(key);

  if (result != ERROR_SUCCESS) {
    fprintf(stderr, "Failed to register the startup entry.\n");
    return EXIT_FAILURE;
  }
  printf("Startup entry registered successfully.\n");
  return EXIT_SUCCESS;
}

static int remove_startup(void) {
  if (!startup_is_registered()) {
    printf("Already removed.\n");
    return EXIT_SUCCESS;
  }

  HKEY key;
  if (RegOpenKeyExA(HKEY_CURRENT_USER, STARTUP_REG_PATH, 0, KEY_SET_VALUE, &key) !=
      ERROR_SUCCESS) {
    fprintf(stderr, "Failed to open the startup registry key.\n");
    return EXIT_FAILURE;
  }
  LONG result = RegDeleteValueA(key, APP_NAME);
  RegCloseKey(key);

  if (result != ERROR_SUCCESS) {
    fprintf(stderr, "Failed to remove the startup entry.\n");
    return EXIT_FAILURE;
  }
  printf("Startup entry removed successfully.\n");
  return EXIT_SUCCESS;
}

// ---------------------------------------------------------------------------
// help / version
// ---------------------------------------------------------------------------

static int show_version(void) {
  printf("%s\n", APP_VERSION);
  return EXIT_SUCCESS;
}

static int show_help(void) {
  printf("Usage:\n");
  printf("  cm on                    - Start mapper process\n");
  printf("  cm off                   - Terminate mapper process\n\n");

  printf("  cm status | s            - Check mapper status\n\n");

  printf("  cm env | e               - Manage the user PATH variable\n");
  printf("      --add     | -a       - Add this directory to PATH\n");
  printf("      --remove  | -r       - Remove this directory from PATH\n\n");

  printf("  cm startup | st          - Manage auto-start at boot\n");
  printf("      --add     | -a       - Register the mapper to start at boot\n");
  printf("      --remove  | -r       - Remove the mapper from startup\n\n");

  printf("  cm --help    | -h        - Show this help message\n");
  printf("  cm --version | -v        - Show version info\n");
  return EXIT_SUCCESS;
}

int show_help_invalid(void) {
  printf("Invalid command. Use '--help' to see available options.\n");
  return EXIT_FAILURE;
}

// ---------------------------------------------------------------------------
// Command table (consumed by packages/common/main.c)
// ---------------------------------------------------------------------------

const CliCommand cli_commands[] = {
    {"on", "on", NULL, NULL, on_runner},
    {"off", "off", NULL, NULL, off_runner},
    {"status", "s", NULL, NULL, show_status},
    {"env", "e", "--add", "-a", add_env},
    {"env", "e", "--remove", "-r", remove_env},
    {"startup", "st", "--add", "-a", add_startup},
    {"startup", "st", "--remove", "-r", remove_startup},
    {"--help", "-h", NULL, NULL, show_help},
    {"--version", "-v", NULL, NULL, show_version},
    {NULL, NULL, NULL, NULL, NULL},
};
