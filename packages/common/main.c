#include <string.h>

#include "cli.h"

// Returns 1 when `arg` equals the given name or alias (either may be NULL).
static int matches(const char* arg, const char* name, const char* alias) {
  return (name && strcmp(arg, name) == 0) || (alias && strcmp(arg, alias) == 0);
}

int main(int argc, char* argv[]) {
  if (argc == 2) {
    for (const CliCommand* c = cli_commands; c->handler; c++) {
      if (!c->option && matches(argv[1], c->name, c->alias)) {
        return c->handler();
      }
    }
  } else if (argc == 3) {
    for (const CliCommand* c = cli_commands; c->handler; c++) {
      if (!c->option) {
        continue;
      }
      // Accept both `cm startup --add` and `cm --add startup`.
      if ((matches(argv[1], c->name, c->alias) && matches(argv[2], c->option, c->option_alias)) ||
          (matches(argv[2], c->name, c->alias) && matches(argv[1], c->option, c->option_alias))) {
        return c->handler();
      }
    }
  }

  return show_help_invalid();
}
