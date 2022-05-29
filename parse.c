#include "parse.h"

#include "mpc.h"

static const char* GRAMMAR =
    "\
    number      : /[+-]?[0-9]*\\.[0-9]+/ | /[+-]?[0-9]+\\.[0-9]*/ | /[+-]?[0-9]+/ ; \
    operator    : '+' | '-' | '*' | '/' | '%' | \
                  \"add\" | \"sub\" | \"mul\" | \"div\" | \"mod\" ; \
    expression  : <number> | '(' <operator> <expression>+ ')'; \
    program     : /^/ <operator> <expression>+ /$/ ; \
    ";

void parser_init(parser* p) {
    p->num = mpc_new("number");
    p->op = mpc_new("operator");
    p->expr = mpc_new("expression");
    p->prog = mpc_new("program");

    mpca_lang(MPCA_LANG_DEFAULT, GRAMMAR, p->num, p->op, p->expr, p->prog);
}

void parser_dispose(parser* p) {
    mpc_cleanup(4, p->num, p->op, p->expr, p->prog);
}

int parser_parse(parser* p, char* input, result* r) {
    return mpc_parse("<stdin>", input, p->prog, &(r->res));
}

void result_print_tree(result* r) {
    mpc_ast_print(r->res.output);
}

void result_print_error(result* r) {
    mpc_err_print(r->res.error);
}

void result_dispose_tree(result* r) {
    mpc_ast_delete(r->res.output);
}

void result_dispose_error(result* r) {
    mpc_err_delete(r->res.error);
}
