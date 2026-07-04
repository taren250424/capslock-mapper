#ifndef APP_H
#define APP_H

#define APP_NAME "cm"
#define APP_VERSION "v2.0.0"

// Installed location of the resident mapper (see deploy/install.sh).
#define RUNNER_PATH "/usr/local/bin/cm_runner"

// Written by the daemonized runner path in process.c; read by status/off.
#define PID_FILE "/run/cm.pid"

// systemd unit used by `cm startup --add/--remove`.
#define SERVICE_NAME "cm.service"
#define SERVICE_PATH "/etc/systemd/system/cm.service"

#endif
