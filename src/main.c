#include <string.h>

#include "parse.h"
#include "repl.h"
#include "test.h"

int main(int argc, char** argv) {
    parser p;

    parser_init(&p);

    if (argc > 1 && strcmp(argv[1], "test") == 0) {
        run_test(&p);
    } else {
        run_repl(&p);
    }

    parser_dispose(&p);

    return 0;
}
