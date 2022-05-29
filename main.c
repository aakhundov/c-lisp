#include "mpc.h"
#include "repl.h"

static const char* GRAMMAR =
    "\
    number      : /-?[0-9]*/ '.' /[0-9]+/ | /-?[0-9]+/ ; \
    operator    : '+' | '-' | '*' | '/' | '%' | \
                  \"add\" | \"sub\" | \"mul\" | \"div\" | \"mod\" ; \
    expression  : <number> | '(' <operator> <expression>+ ')'; \
    program     : /^/ <operator> <expression>+ /$/ ; \
    ";

int main(int argc, char** argv) {
    mpc_parser_t* number = mpc_new("number");
    mpc_parser_t* operator= mpc_new("operator");
    mpc_parser_t* expression = mpc_new("expression");
    mpc_parser_t* program = mpc_new("program");

    mpca_lang(MPCA_LANG_DEFAULT, GRAMMAR, number, operator, expression, program);

    run_repl(program);

    mpc_cleanup(4, number, operator, expression, program);

    return 0;
}
