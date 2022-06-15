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
static const char* clear_commands[] = {"clear", "clr", "clrscr"};
static const char* env_commands[] = {"env", "environment"};

static const char** commands[] = {
    exit_commands,
    clear_commands,
    env_commands};

static const size_t command_sizes[] = {
    sizeof(exit_commands) / sizeof(char*),
    sizeof(clear_commands) / sizeof(char*),
    sizeof(env_commands) / sizeof(char*)};

typedef enum {
    COMMAND_EXIT = 0,
    COMMAND_CLEAR = 1,
    COMMAND_ENV = 2,
    COMMAND_OTHER = -1
} command_type;

static void get_input(char* input) {
    char* running = input;

    int done = 0;
    while (!done) {
        char* line = readline("> ");
        size_t len = strlen(line);
        if (len >= 3 && strcmp(line + len - 3, "...") == 0) {
            line[len - 3] = ' ';
            line[len - 2] = '\0';
        } else {
            done = 1;
        }

        running += sprintf(running, "%s", line);
        free(line);
    }
}

static command_type get_command_type(char* line) {
    size_t num_command_types = sizeof(commands) / sizeof(char**);
    for (size_t i = 0; i < num_command_types; i++) {
        size_t num_commands = command_sizes[i];
        for (size_t j = 0; j < num_commands; j++) {
            if (strcmp(line, commands[i][j]) == 0) {
                return i;
            }
        }
    }

    return COMMAND_OTHER;
}

static void process_repl_command(environment* env, char* input, char* output) {
    add_history(input);

    value* v = value_parse(input);
    if (v->type != VALUE_ERROR) {
        value* e = value_evaluate(v, env);
        value_dispose(v);
        v = e;
    }

    value_to_str(v, output);
    value_dispose(v);
}

void run_repl() {
    printf("mylisp version 0.0.1\n");
    printf("type in \"q\" to quit\n\n");

    environment env;
    environment_init(&env);
    environment_register_builtins(&env);

    int stop = 0;
    char input[65536];
    char output[65536];

    while (!stop) {
        get_input(input);
        switch (get_command_type(input)) {
            case COMMAND_EXIT:
                stop = 1;
                break;
            case COMMAND_CLEAR:
                printf("\e[1;1H\e[2J");
                break;
            case COMMAND_ENV:
                environment_to_str(&env, output);
                printf("%s", output);
                break;
            default:
                process_repl_command(&env, input, output);
                printf("%s\n", output);
        }
    }

    environment_dispose(&env);

    printf("\nbye!\n");
}
