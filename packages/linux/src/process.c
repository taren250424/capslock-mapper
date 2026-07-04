// Command handlers for the Linux CLI. The dispatch loop lives in
// packages/common/main.c; this file supplies the command table and the
// platform-specific work.
//
// NOTE: refactored on Windows without a Linux machine at hand.
// Behavior is intended to be identical to v1 (plus the reg->startup rename);
// see packages/linux/TODO.md for the verification checklist.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "app.h"
#include "cli.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// The runner needs root to grab /dev/input and create uinput devices, so the
// commands that manage it do too.
static int require_root(void) {
  if (getuid() != 0) {
    fprintf(stderr, "Error: this command must be run with sudo.\n");
    return 0;
  }
  return 1;
}

// Reads the daemon PID from PID_FILE. Returns -1 when absent or unreadable.
static int read_pid(void) {
  FILE* fp = fopen(PID_FILE, "r");
  if (!fp) {
    return -1;
  }
  int pid = -1;
  if (fscanf(fp, "%d", &pid) != 1) {
    pid = -1;
  }
  fclose(fp);
  return pid;
}

static int write_pid(int pid) {
  FILE* fp = fopen(PID_FILE, "w");
  if (!fp) {
    return -1;
  }
  fprintf(fp, "%d\n", pid);
  fclose(fp);
  return 0;
}

// /proc existence works without privileges, unlike kill(pid, 0) which would
// report EPERM for the root-owned runner when called from a plain user
// (status must stay sudo-free).
static int pid_alive(int pid) {
  char path[64];
  snprintf(path, sizeof(path), "/proc/%d", pid);
  return access(path, F_OK) == 0;
}

static int service_active(void) {
  // Returns 0 only when the unit is active.
  return system("systemctl is-active --quiet " SERVICE_NAME) == 0;
}

// The runner counts as running when either the standalone daemon (PID file)
// or the systemd service is alive.
static int runner_is_running(void) {
  int pid = read_pid();
  if (pid > 0 && pid_alive(pid)) {
    return 1;
  }
  return service_active();
}

// ---------------------------------------------------------------------------
// on / off / status
// ---------------------------------------------------------------------------

static int on_runner(void) {
  if (!require_root()) {
    return EXIT_FAILURE;
  }
  if (runner_is_running()) {
    printf("Program is already running.\n");
    return EXIT_SUCCESS;
  }
  if (access(RUNNER_PATH, X_OK) != 0) {
    fprintf(stderr, "Cannot find runner binary at %s.\n", RUNNER_PATH);
    return EXIT_FAILURE;
  }

  // Daemonize with the classic double fork:
  //   parent -> child (new session leader) -> grandchild (the daemon).
  pid_t pid = fork();
  if (pid < 0) {
    perror("Fork failed");
    return EXIT_FAILURE;
  }
  if (pid > 0) {
    // Parent. The first child exits almost immediately during
    // daemonization, so its PID is NOT written to the PID file here;
    // the grandchild records its own PID once it is fully set up.
    printf("Program has started.\n");
    return EXIT_SUCCESS;
  }

  // First child: become a session leader to detach from the terminal.
  if (setsid() < 0) {
    exit(1);
  }

  // Fork again so the final process is not a session leader and can never
  // reacquire a controlling terminal (System V rule).
  pid_t pid2 = fork();
  if (pid2 > 0) {
    exit(0);
  }
  if (pid2 < 0) {
    exit(1);
  }

  // Grandchild: the actual daemon. Record the real worker PID.
  write_pid(getpid());

  // Detach stdio: close the terminal-connected descriptors and point
  // 0/1/2 at /dev/null so stray I/O from any library cannot crash us.
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
  open("/dev/null", O_RDONLY); // fd 0
  open("/dev/null", O_WRONLY); // fd 1
  open("/dev/null", O_WRONLY); // fd 2

  // Replace this process image with the runner. On success execv never
  // returns; on failure there is nothing useful left to do but exit.
  execv(RUNNER_PATH, (char*[]){(char*)RUNNER_PATH, NULL});
  exit(1);
}

static int off_runner(void) {
  if (!require_root()) {
    return EXIT_FAILURE;
  }

  int stopped = 0;

  if (service_active()) {
    if (system("systemctl stop " SERVICE_NAME) != 0) {
      fprintf(stderr, "Failed to stop the systemd service.\n");
      return EXIT_FAILURE;
    }
    printf("Service stopped.\n");
    stopped = 1;
  }

  int pid = read_pid();
  if (pid > 0 && pid_alive(pid)) {
    if (kill(pid, SIGTERM) != 0) {
      perror("Failed to stop the runner");
      return EXIT_FAILURE;
    }
    unlink(PID_FILE);
    printf("Process stopped.\n");
    stopped = 1;
  }

  if (!stopped) {
    printf("Program was not running.\n");
  }
  return EXIT_SUCCESS;
}

static int show_status(void) {
  int running = runner_is_running();
  int registered = access(SERVICE_PATH, F_OK) == 0;

  printf("[Running status]    %s\n",
         running ? "\033[0;32mRunning.\033[0m" : "\033[0;31mNot running.\033[0m");
  printf("[Startup status]    %s\n",
         registered ? "\033[0;32mRegistered.\033[0m" : "\033[0;31mNot registered.\033[0m");
  return EXIT_SUCCESS;
}

// ---------------------------------------------------------------------------
// startup --add / --remove (systemd service)
// ---------------------------------------------------------------------------

static int add_startup(void) {
  if (!require_root()) {
    return EXIT_FAILURE;
  }

  FILE* fp = fopen(SERVICE_PATH, "w");
  if (!fp) {
    perror("Failed to create the service file");
    return EXIT_FAILURE;
  }
  fprintf(fp, "[Unit]\n");
  fprintf(fp, "Description=CapsLock Mapper Service\n");
  fprintf(fp, "After=network.target\n\n");
  fprintf(fp, "[Service]\n");
  fprintf(fp, "ExecStart=%s\n", RUNNER_PATH);
  fprintf(fp, "User=root\n\n");
  fprintf(fp, "[Install]\n");
  fprintf(fp, "WantedBy=multi-user.target\n");
  fclose(fp);

  if (system("systemctl daemon-reload") != 0 ||
      system("systemctl enable " SERVICE_NAME) != 0 ||
      system("systemctl start " SERVICE_NAME) != 0) {
    fprintf(stderr, "A systemctl command failed; the service may be partially registered.\n");
    return EXIT_FAILURE;
  }

  printf("Service registered and started successfully.\n");
  return EXIT_SUCCESS;
}

static int remove_startup(void) {
  if (!require_root()) {
    return EXIT_FAILURE;
  }

  // Stop/disable may legitimately fail when the unit was never started or
  // enabled, so their results are intentionally not treated as errors.
  if (system("systemctl stop " SERVICE_NAME) != 0) {
    // best effort
  }
  if (system("systemctl disable " SERVICE_NAME) != 0) {
    // best effort
  }

  if (unlink(SERVICE_PATH) != 0) {
    perror("Failed to remove the service file");
    return EXIT_FAILURE;
  }
  if (system("systemctl daemon-reload") != 0) {
    fprintf(stderr, "systemctl daemon-reload failed.\n");
    return EXIT_FAILURE;
  }

  printf("Service removed successfully.\n");
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
  printf("  sudo cm on               - Start mapper process\n");
  printf("  sudo cm off              - Terminate mapper process\n\n");

  printf("  cm status | s            - Check mapper status\n\n");

  printf("  sudo cm startup | st     - Manage auto-start at boot (systemd)\n");
  printf("      --add     | -a       - Register and start the service\n");
  printf("      --remove  | -r       - Stop and unregister the service\n\n");

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
    {"startup", "st", "--add", "-a", add_startup},
    {"startup", "st", "--remove", "-r", remove_startup},
    {"--help", "-h", NULL, NULL, show_help},
    {"--version", "-v", NULL, NULL, show_version},
    {NULL, NULL, NULL, NULL, NULL},
};
