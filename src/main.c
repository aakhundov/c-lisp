#include <string.h>

#include "env.h"
#include "eval.h"
#include "parse.h"
#include "repl.h"
#include "test.h"

int main(int argc, char** argv) {
    parser p;
    environment e;

    parser_init(&p);
    environment_init(&e);
    environment_register_builtins(&e);

    if (argc > 1 && strcmp(argv[1], "test") == 0) {
        run_test(&p, &e);
    } else {
        run_repl(&p, &e);
    }

    environment_dispose(&e);
    parser_dispose(&p);

    return 0;
}
