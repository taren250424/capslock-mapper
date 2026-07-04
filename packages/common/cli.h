#ifndef CLI_H
#define CLI_H

// Shared CLI dispatch used by both the Windows and Linux packages.
//
// Each platform's process.c defines `cli_commands`, a flat table where one
// row is one invocable action:
//   - a bare command:           { "on", "on", NULL, NULL, on_runner }
//   - a command with an option: { "startup", "st", "--add", "-a", add_startup }
// The table ends with a row whose handler is NULL.
//
// Matching rules (implemented in main.c):
//   cm <cmd>       -> first row without an option whose name/alias matches
//   cm <cmd> <opt> -> first row whose name/alias AND option/option_alias both
//                     match; the two arguments may appear in either order
//                     (`cm --add startup` works too).
// Anything else falls through to show_help_invalid().

typedef int (*CliHandler)(void);

typedef struct {
  const char* name;         // primary command name, e.g. "startup"
  const char* alias;        // short form, e.g. "st" (same as name if none)
  const char* option;       // option long form, e.g. "--add"; NULL for bare commands
  const char* option_alias; // option short form, e.g. "-a"
  CliHandler handler;
} CliCommand;

// Defined by each platform's process.c.
extern const CliCommand cli_commands[];

// Fallback for unrecognized input; defined by each platform's process.c.
int show_help_invalid(void);

#endif
