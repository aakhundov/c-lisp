#include "parse.h"

#include <string.h>

#include "mpc.h"

typedef struct {
    mpc_parser_t* num;
    mpc_parser_t* sym;
    mpc_parser_t* spec;
    mpc_parser_t* str;
    mpc_parser_t* cmnt;
    mpc_parser_t* sexpr;
    mpc_parser_t* qexpr;
    mpc_parser_t* expr;
    mpc_parser_t* prog;
} parser;

static const char* GRAMMAR =
    "\
    number         : /[+-]?[0-9]*\\.[0-9]*/ | \
                     /[+-]?[0-9]+\\.[0-9]*/ | \
                     /[+-]?[0-9]+/ ; \
    symbol         : /[a-zA-Z0-9_+\\-*\\/%\\^\\\\=<>!&|\\?]+/ ; \
    special        : \"#true\" | \"#false\" | \"#null\"; \
    string         : /\"(\\\\.|[^\"])*\"/ ; \
    comment        : /;[^\\r\\n]*/ ; \
    sexpr          : '(' <expr>* ')' ; \
    qexpr          : '{' <expr>* '}' ; \
    expr           : <number> | <symbol> | <special> | <string> | \
                     <comment> | <sexpr> | <qexpr> ; \
    program        : /^/ <expr>* /$/ ; \
    ";

static parser* p;

static parser_tree wrap_mpc_tree(mpc_ast_t* ast) {
    parser_tree t;

    t.tag = ast->tag;
    t.content = ast->contents;
    t.num_children = ast->children_num;
    t.ast = ast;

    return t;
}

void parser_init() {
    if (p == NULL) {
        p = malloc(sizeof(parser));

        p->num = mpc_new("number");
        p->sym = mpc_new("symbol");
        p->spec = mpc_new("special");
        p->str = mpc_new("string");
        p->cmnt = mpc_new("comment");
        p->sexpr = mpc_new("sexpr");
        p->qexpr = mpc_new("qexpr");
        p->expr = mpc_new("expr");
        p->prog = mpc_new("program");

        mpca_lang(
            MPCA_LANG_DEFAULT, GRAMMAR,
            p->num, p->sym, p->spec, p->str, p->cmnt, p->sexpr, p->qexpr, p->expr, p->prog);
    }
}

void parser_dispose() {
    if (p != NULL) {
        mpc_cleanup(
            9,
            p->num, p->sym, p->spec, p->str, p->cmnt, p->sexpr, p->qexpr, p->expr, p->prog);

        free(p);
        p = NULL;
    }
}

int parser_parse(char* input, parser_result* r) {
    return mpc_parse("<stdin>", input, p->prog, &(r->res));
}

parser_tree parser_result_get_tree(parser_result* r) {
    return wrap_mpc_tree(r->res.output);
}

int parser_result_get_error(parser_result* r, char* buffer) {
    char* err = mpc_err_string(r->res.error);
    err[strlen(err) - 1] = '\0';  // remove \n
    int result = sprintf(buffer, "%s", err);
    free(err);

    return result;
}

void parser_result_dispose_tree(parser_result* r) {
    mpc_ast_delete(r->res.output);
}

void parser_result_dispose_error(parser_result* r) {
    mpc_err_delete(r->res.error);
}

parser_tree parser_tree_get_child(parser_tree* t, size_t index) {
    return wrap_mpc_tree(t->ast->children[index]);
}
