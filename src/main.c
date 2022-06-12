#include <string.h>

#include "parse.h"
#include "repl.h"
#include "test.h"

int main(int argc, char** argv) {
    parser_init();

    if (argc > 1 && strcmp(argv[1], "test") == 0) {
        run_test();
    } else {
        run_repl();
    }

    parser_dispose();

    return 0;
}
