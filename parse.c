#include "parse.h"

#include "mpc.h"

static const char* GRAMMAR =
    "\
    number         : /[+-]?[0-9]*\\.[0-9]*/ | \
                     /[+-]?[0-9]+\\.[0-9]*/ | \
                     /[+-]?[0-9]+/ ; \
    symbol         : '+' | '-' | '*' | '/' | '%' | '^' | \
                     \"add\" | \"sub\" | \"mul\" | \"div\" | \"mod\" | \
                     \"pow\" | \"min\" | \"max\" | \"fake\" ; \
    sexpr          : '(' <expr>* ')' ; \
    expr           : <number> | <symbol> | <sexpr> ; \
    program        : /^/ <expr>* /$/ ; \
    ";

static tree wrap_mpc_tree(mpc_ast_t* ast) {
    tree t;

    t.tag = ast->tag;
    t.content = ast->contents;
    t.num_children = ast->children_num;
    t.ast = ast;

    return t;
}

void parser_init(parser* p) {
    p->num = mpc_new("number");
    p->sym = mpc_new("symbol");
    p->sexpr = mpc_new("sexpr");
    p->expr = mpc_new("expr");
    p->prog = mpc_new("program");

    mpca_lang(MPCA_LANG_DEFAULT, GRAMMAR, p->num, p->sym, p->sexpr, p->expr, p->prog);
}

void parser_dispose(parser* p) {
    mpc_cleanup(5, p->num, p->sym, p->sexpr, p->expr, p->prog);
}

int parser_parse(parser* p, char* input, result* r) {
    return mpc_parse("<stdin>", input, p->prog, &(r->res));
}

tree result_get_tree(result* r) {
    return wrap_mpc_tree(r->res.output);
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

tree tree_get_child(tree* t, size_t index) {
    return wrap_mpc_tree(t->ast->children[index]);
}
