#include "repl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "edit.h"
#include "mpc.h"

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

void run_repl(mpc_parser_t* program) {
    puts("CLisp version 0.0.1");
    puts("Type \"quit\" to quit\n");

    int stop = 0;
    while (!stop) {
        char* input = readline("clisp> ");

        if (is_exit_command(input)) {
            stop = 1;
        } else {
            add_history(input);

            mpc_result_t result;
            if (mpc_parse("<stdin>", input, program, &result)) {
                mpc_ast_print(result.output);
                mpc_ast_delete(result.output);
            } else {
                mpc_err_print(result.error);
                mpc_err_delete(result.error);
            }
        }

        free(input);
    }

    printf("\nBye!\n");
}
