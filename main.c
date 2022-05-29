#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "edit.h"

static const char* exit_commands[] = {"exit", "quit", "q"};
static const size_t n_exit_commands = sizeof(exit_commands) / sizeof(char*);

static int is_exit_command(char* line) {
    for (size_t i = 0; i < n_exit_commands; i++) {
        if (strcmp(line, exit_commands[i]) == 0) {
            return 1;
        }
    }

    return 0;
}

int main(int argc, char** argv) {
    puts("CLisp version 0.0.1");
    puts("Type \"quit\" to quit\n");

    int stop = 0;
    while (!stop) {
        char* input = readline("clisp> ");

        if (is_exit_command(input)) {
            stop = 1;
        } else {
            add_history(input);
            printf("echo: \"%s\"\n", input);
        }

        free(input);
    }

    printf("\nBye!\n");

    return 0;
}
