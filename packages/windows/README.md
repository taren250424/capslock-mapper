# CapsLock Mapper (Windows Version)

A small Windows utility that maps the Caps Lock key to simulate a left mouse click, helping reduce strain on your right wrist. It offers fast and simple command-line options, making it especially useful for developers who are comfortable with the CLI.


## Features

- **Simulate left mouse clicks using the Caps Lock key**  
Helps reduce on the right wrist strain by allowing you to click without using the mouse button.

- **Caps Lock function is overridden while the program is running**  
To type capital letters, use the Shift key instead of Caps Lock during operation.

- **Quickly enable or disable with simple CLI commands**  
In cases where holding Shift isn't practical—like writing in all caps—you can easily pause and resume the program using 'cm off' and 'cm on'.

- **No administrator privileges required**  
The program runs without elevation, so no UAC prompts appear. As a result, key mapping will **not work in elevated windows** (e.g., installers or applications running as administrator).


## Commands

The utility supports the following commands:

```bash
Usage:
  cm on                       - Start mapper process
  cm off                      - Terminate mapper process
  
  cm status | cm s            - Check mapper status

  cm env | cm e               - Manage environment variables
      --add     | -a          - Add environment variables
      --remove  | -r          - Remove environment variables

  cm reg | cm r               - Manage registry startup entry
      --add     | -a          - Add mapper to startup (via registry)
      --remove  | -r          - Remove mapper from startup

  cm --help    | -h           - Show help message
  cm --version | -v           - Show version info
```


## Example Usage

```bash
cm on                         # Start mapper process
cm off                        # Terminate mapper process

cm status                     # Check mapper status
cm s                          # Alias for status

cm env --add                  # Add environment variables
cm e -r                       # Remove environment variables (alias for env)

cm reg --add                  # Register mapper to run at startup
cm r -r                       # Remove mapper from registry (alias for reg)

cm --help                     # Show help message
cm -v                         # Show version info
```


## Installation

1. **Download** the latest version of `cm-windows.zip` from the [release page](https://github.com/Hyeonnam-J/Windows-CapsLock-Mapper/releases)
2. **Extract** the contents of the zip file to your desired directory.


## Setup

After extracting the files, follow these steps to set up the application:

1. **Add environment variable**:
   Navigate to the extracted directory and run the following command to add the necessary environment variable:

```bash
.\cm env -add
```

After this, you will be able to use the cm command in your terminal. If the command doesn't work immediately, try restarting your terminal.
To remove the environment variable, you can run:

```bash
cm env --remove
```

2. **Enable automatic key mapping on boot**:
   To make sure the key mapping is applied automatically after every system boot, run:

```bash
cm reg --add
```

This will add the necessary registry entry. If you want to disable this feature later, run:

```bash
cm reg --remove
```