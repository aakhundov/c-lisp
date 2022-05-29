#include "parse.h"
#include "repl.h"

int main(int argc, char** argv) {
    parser p;

    parser_init(&p);

    run_repl(&p);

    parser_dispose(&p);

    return 0;
}
