#ifndef PARSE_H_
#define PARSE_H_

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

typedef struct {
    mpc_result_t res;
} result;

typedef struct {
    char* tag;
    char* content;
    size_t num_children;
    mpc_ast_t* ast;
} tree;

void parser_init(parser* p);
void parser_dispose(parser* p);
int parser_parse(parser* p, char* input, result* r);

tree result_get_tree(result* r);
void result_print_tree(result* r);
void result_print_error(result* r);
void result_dispose_tree(result* r);
void result_dispose_error(result* r);

tree tree_get_child(tree* t, size_t index);

#endif  // PARSE_H_
