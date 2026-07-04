<!--
NOTE (v2.0.0): this README still documents the released v1 binaries on
purpose. Update it together with the v2 Linux release; the only user-facing
change is the command rename `service | srv` -> `startup | st`, plus the
CMake build (see packages/linux/TODO.md).
-->

# CapsLock Mapper (Linux Version)

A small Linux utility that maps the Caps Lock key to simulate a left mouse click, helping reduce strain on your right wrist. It offers fast and simple command-line options, making it especially useful for developers who are comfortable with the CLI.


## Features

- **Simulate left mouse clicks using the Caps Lock key**  
Helps reduce on the right wrist strain by allowing you to click without using the mouse button.

- **Caps Lock function is overridden while the program is running**  
To type capital letters, use the Shift key instead of Caps Lock during operation.

- **Quickly enable or disable with simple CLI commands**  
In cases where holding Shift isn't practical—like writing in all caps—you can easily pause and resume the program using 'sudo cm off' and 'sudo cm on'.


## Commands

The utility supports the following commands:

```bash
Usage:
  sudo cm on                       - Start mapper process
  sudo cm off                      - Terminate mapper process

  cm status | cm s                 - Check mapper status

  sudo cm service | srv            - Manage systemd service
    --add    | -a                  - Register mapper to start on boot
    --remove | -r                  - Remove mapper from startup

  cm --help    | -h                - Show help message
  cm --version | -v                - Show version info
```


## Example Usage

```bash
sudo cm on                         # Start mapper process
sudo cm off                        # Terminate mapper process

cm status                          # Check mapper status
cm s                               # Alias for status

cm service --add                   # Add service system
cm srv -r                          # Remove service system (alias for service)

cm --help                          # Show help message
cm -v                              # Show version info
```


## Download

1. **Download** the latest version of `cm-linux.tar.gz` from the [release page](https://github.com/Hyeonnam-J/Windows-CapsLock-Mapper/releases)
2. **Extract** the contents of the zip file to your desired directory.


## Installation

To install the program globally, navigate to the extracted directory and run the install script:

```bash
sudo sh install.sh
```

This will move the executable to /usr/local/bin, allowing you to run the 'cm' command from any terminal.  

If you want to uninstall the program, run:

```bash
sudo sh uninstall.sh
```


## Setup

After installation, you can choose how you want to use the application depending on your preference.

Automatic Key Mapping on Boot (Optional) If you want the mapper to start automatically every time you boot your Linux system, you can register it as a system service. This is completely optional. To register and enable the service, run:

```bash
sudo cm srv --add
```

To unregister and disable the service later, run:

```bash
sudo cm srv --remove
```