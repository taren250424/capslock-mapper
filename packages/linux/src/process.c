#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "constants/app_constants.h"
#include "constants/result_constants.h"

#include "process.h"

int is_running() {
    // 1. PID-based check: Verify if a standalone process exists using the PID file.
    int pid = read_pid();
    if (pid > 0) {
        char path[256];
        snprintf(path, sizeof(path), "/proc/%d", pid);
        
        // access() with F_OK checks for the existence of the /proc/[pid] directory.
        if (access(path, F_OK) == 0) {
            return IS_RUNNING;
        }
    }

    // 2. Service-based check: Query systemd to see if 'cm.service' is active.
    // 'systemctl is-active --quiet' returns 0 only if the service is running.
    if (system("systemctl is-active --quiet cm.service") == 0) {
        return IS_RUNNING;
    }

    return IS_NOT_RUNNING;
}

int read_pid() {
    FILE *fp = fopen(PID_FILE, "r");
    if (!fp) return IS_NOT_RUNNING;
    
    int pid;
    if (fscanf(fp, "%d", &pid) != 1) {
        fclose(fp);
        return FAIL;
    }
    
    fclose(fp);
    return pid;
}

int write_pid(int pid) {
    FILE *fp = fopen(PID_FILE, "w");
    if (!fp) {
        perror("Cannot create PID file");
        return FAIL;
    }
    
    fprintf(fp, "%d\n", pid);
    fclose(fp);
    return SUCCESS;
}

int on_runner() {
    if (getuid() != 0) {
        fprintf(stderr, "Error: This command must be run with sudo.\n");
        return FAIL;
    }

    if (is_running() == IS_RUNNING) {
        printf("Already running.\n");
        return SUCCESS;
    }
    
    // Get absolute path of runner.
    char runner_abs_path[4096];
    if (realpath(APP_RUNNER_PATH, runner_abs_path) == NULL) {
        perror("Cannot find runner binary");
        return FAIL;
    }
    
    // Fork the current process to create a child process.
    // After fork(), two identical processes run simultaneously.
    pid_t pid = fork();
    
    // Case 1: Fork failed. (e.g., system-wide process limit reached)
    if (pid < 0) {
        perror("Fork failed");
        return FAIL;
    }
    
    // Case 2: Parent process.
    // fork() returns the PID of the newly created child to the parent.
    if (pid > 0) {
        // DO NOT write the first child's PID to the file.
        // During the daemonization process (Double Fork), this first child 
        // will exit almost immediately. The actual long-running worker 
        // is the 'grandchild' created by this child.
        // 
        // If the parent writes the child's PID here, the system will later 
        // attempt to manage (status/stop) a process that no longer exists, 
        // leading to "stale PID" logic errors.
        // The grandchild must write its own true PID once it is fully initialized.
        // write_pid(pid);
				
        printf("Program has started. (PID: %d)\n", pid);
        return SUCCESS;
    }
        
    // Create a new session and set the current process as the session leader.
    // This detaches the process from the controlling terminal (TTY).
    if (setsid() < 0) {
        // If setsid fails, the process cannot safely run as a daemon.
        exit(1);
    }
    
    // 2. Perform a "Double Fork" to ensure complete daemonization.
    // By forking a second time, the grandchild process is guaranteed 
    // NOT to be a session leader. Under System V Unix rules, only a 
    // session leader can acquire a controlling terminal (TTY).
    pid_t pid2 = fork();
    if (pid2 > 0) {
        exit(0);
    }
    if (pid2 < 0) {
        exit(1);
    }
    
    // 3. Grandchild process: The actual daemon.
    
    // The previous PID (written by the grandparent) was just a temporary placeholder 
    // for the first child, who wasn't the "real" worker and has now exited. 
    // Now that the grandchild—the actual protagonist of this service—is ready 
    // and all daemonization steps are complete, we overwrite the file with 
    // the grandchild's true PID using getpid().
    
    // This marks the official "Grand Opening" of the daemon service.
    write_pid(getpid());
    
    // 4. Close standard file descriptors (stdin, stdout, stderr).
    // These are still connected to the controlling terminal. Closing them
    // ensures the daemon doesn't try to use the disconnected terminal.
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    // 5. Redirect stdin, stdout, and stderr to /dev/null.
    // By opening /dev/null immediately after closing 0, 1, and 2, the system
    // automatically assigns these lowest-available descriptors to the bit-bucket.
    // This prevents crashes if the program or a library attempts I/O.
    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);
    open("/dev/null", O_WRONLY);
    
    // 6. Execute the runner program.
    // execv replaces the current process image with the new program.
    // If successful, this process starts running the new code and never returns here.
    execv(runner_abs_path, (char *[]){runner_abs_path, NULL});
    
    // If we reach this point, it means execv has FAILED (e.g., file not found).
    // Since the daemon cannot fulfill its purpose without the runner, 
    // we must terminate the process immediately to avoid leaving a "zombie" or 
    // useless idle process in the background.
    exit(1);

    // This return is technically unreachable but kept for structural integrity.
    return FAIL;
}

int off_runner() {
    if (getuid() != 0) {
        fprintf(stderr, "Error: This command must be run with sudo.\n");
        return FAIL;
    }

    int removed = 0;

    if (system("systemctl is-active --quiet cm.service") == 0) {
        system("systemctl stop cm.service");
        printf("Service stopped.\n");
        removed = 1;
    }

    int pid = read_pid();
    if (pid > 0 && kill(pid, 0) == 0) {
        kill(pid, SIGTERM);
        unlink(PID_FILE);
        printf("Process stopped.\n");
        removed = 1;
    }

    if (!removed) {
        printf("Program was not running.\n");
    }
    return SUCCESS;
}

int show_status() {
    int running = is_running();
    int service_registered = (access(SERVICE_PATH, F_OK) == 0);

    printf("[Running status]    %s\n", 
        running == IS_RUNNING ? "\033[0;32mRunning.\033[0m" : "\033[0;31mNot running.\033[0m");
    
    printf("[Service status]    %s\n", 
           service_registered ? "\033[0;32mRegistered.\033[0m" : "\033[0;31mNot registered.\033[0m");

    return 0;
}

int add_service() {
    if (getuid() != 0) {
        printf("Error: Privilege required (use sudo).\n");
        return -1;
    }

		FILE *fp = fopen(SERVICE_PATH, "w");
    if (!fp) {
        perror("Failed to create service file");
        return FAIL;
    }

    fprintf(fp, "[Unit]\n");
    fprintf(fp, "Description=CapsLock Mapper Service\n");
    fprintf(fp, "After=network.target\n\n");
    fprintf(fp, "[Service]\n");
    fprintf(fp, "ExecStart=%s\n", APP_RUNNER_PATH);
    fprintf(fp, "User=root\n\n");
    fprintf(fp, "[Install]\n");
    fprintf(fp, "WantedBy=multi-user.target\n");
    fclose(fp);

    system("systemctl daemon-reload");
    system("systemctl enable cm.service");
    system("systemctl start cm.service");

    printf("Service registered and started successfully.\n");
    return 0;
}

int remove_service() {
    if (getuid() != 0) {
        printf("Error: Privilege required (use sudo).\n");
        return -1;
    }

    system("systemctl stop cm.service");
    system("systemctl disable cm.service");
    
    if (unlink(SERVICE_PATH) == 0) {
        system("systemctl daemon-reload");
        printf("Service removed successfully.\n");
        return 0;
    } else {
        perror("Failed to remove service file");
        return -1;
    }
}

int show_version() {
    printf("%s\n", VERSION);
    return 0;
}

int show_help() {
    printf("Usage:\n");
    printf("  on                       - Start mapper process\n");
    printf("  off                      - Terminate mapper process\n\n");
    
    printf("  status | s               - Check mapper status\n\n");

    printf("  service | srv            - Manage systemd service\n");
    printf("      --add     | -a       - Register and start service (boot auto-start)\n");
    printf("      --remove  | -r       - Stop and unregister service\n\n");

    printf("  --help    | -h           - Show this help message\n");
    printf("  --version | -v           - Show version info\n");

    return 0;
}

int show_help_invalid() {
    printf("Invalid command. Use '--help' to see available options.\n");
    return 0;
}

// Check for NULL 'name' values to terminate the list of commands and options.
struct CommandWithOptions commandWithOptions[] = {
    {
        .command = {
            "on",
            "on",
            on_runner
        },
        .options = {
            { NULL, NULL, NULL }
        }
    },
    {
        .command = {
            "off",
            "off",
            off_runner
        },
        .options = {
            { NULL, NULL, NULL }
        }
    },
        {
        .command = {
            "status",
            "s",
            show_status
        },
        .options = {
            { NULL, NULL, NULL }
        }
    },
    {
        .command = {
            NULL,
            NULL,
            NULL
        },
        .options = {
            { "--help", "-h", show_help },
            { "--version", "-v", show_version },
            { NULL, NULL, NULL }
        }
    },
    {
        .command = {
            "service",
            "srv",
            show_help
        },
        .options = {
            { "--add", "-a", add_service },
            { "--remove", "-r", remove_service },
            { NULL, NULL, NULL }
        }
    },
    {
        .command = {
            NULL,
            NULL,
            NULL
        },
        .options = {
            { NULL, NULL, NULL }
        }
    },
};