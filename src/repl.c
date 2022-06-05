#include "repl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "edit.h"
#include "env.h"
#include "eval.h"
#include "parse.h"
#include "value.h"

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

void run_repl(parser* p, environment* env) {
    puts("clisp version 0.0.1");
    puts("enter \"quit\" to quit\n");

    int stop = 0;
    char output[2048];

    while (!stop) {
        char* input = readline("clisp> ");

        if (is_exit_command(input)) {
            stop = 1;
        } else {
            add_history(input);

            result r;
            if (parser_parse(p, input, &r)) {
                tree t = result_get_tree(&r);
                value* v = value_from_tree(&t);
                value* e = value_evaluate(v, env);

                value_to_str(e, output);
                printf("%s\n", output);

                value_dispose(e);
                value_dispose(v);
                result_dispose_tree(&r);
            } else {
                result_print_error(&r);
                result_dispose_error(&r);
            }
        }

        free(input);
    }

    printf("\nbye!\n");
}
