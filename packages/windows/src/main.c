#include <string.h>

#include "process.h"

int handleArgs(char* arg1, char* arg2) {
    for (int i = 0; commandWithOptions[i].command.name != NULL || commandWithOptions[i].options[0].name != NULL; i++) {
        int isArg1Cmd = commandWithOptions[i].command.name && ( (strcmp(arg1, commandWithOptions[i].command.name) == 0) || (strcmp(arg1, commandWithOptions[i].command.alias) == 0) );
        int isArg2Cmd = commandWithOptions[i].command.name && ( (strcmp(arg2, commandWithOptions[i].command.name) == 0) || (strcmp(arg2, commandWithOptions[i].command.alias) == 0) );

        if (isArg1Cmd || isArg2Cmd) {
            char* opt = isArg1Cmd ? arg2 : arg1;

            for (int j = 0; commandWithOptions[i].options[j].name != NULL; j++) {
                if (strcmp(opt, commandWithOptions[i].options[j].name) == 0 || strcmp(opt, commandWithOptions[i].options[j].alias) == 0) {
                    return commandWithOptions[i].options[j].handler();
                }
            }
        }
    }

    return show_help_invalid();
}

int handleArg(char* arg) {
    for (int i = 0; commandWithOptions[i].command.name != NULL || commandWithOptions[i].options[0].name != NULL; i++) {
        if (commandWithOptions[i].command.name != NULL) {
            if (strcmp(arg, commandWithOptions[i].command.name) == 0 || strcmp(arg, commandWithOptions[i].command.alias) == 0) {
                // Command may have a NULL handler, unlike options.
                return commandWithOptions[i].command.handler ? commandWithOptions[i].command.handler() : show_help_invalid();
            }
        } else {
            for (int j = 0; commandWithOptions[i].options[j].name != NULL; j++) {
                if (strcmp(arg, commandWithOptions[i].options[j].name) == 0 || strcmp(arg, commandWithOptions[i].options[j].alias) == 0) {
                    return commandWithOptions[i].options[j].handler();
                }
            }
        }
    }

    return show_help_invalid();
}

int main(int argc, char* argv[]) {
    if (argc == 2) return handleArg(argv[1]);
    else if (argc == 3) return handleArgs(argv[1], argv[2]);
    else return show_help_invalid();
}
