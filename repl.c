#include "repl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "edit.h"
#include "parse.h"

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

void run_repl(parser* p) {
    puts("CLisp version 0.0.1");
    puts("Type \"quit\" to quit\n");

    int stop = 0;
    while (!stop) {
        char* input = readline("clisp> ");

        if (is_exit_command(input)) {
            stop = 1;
        } else {
            add_history(input);

            result r;
            if (parser_parse(p, input, &r)) {
                result_print_tree(&r);
                result_dispose_tree(&r);
            } else {
                result_print_error(&r);
                result_dispose_error(&r);
            }
        }

        free(input);
    }

    printf("\nBye!\n");
}
