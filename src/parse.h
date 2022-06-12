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
} parser_result;

typedef struct {
    char* tag;
    char* content;
    size_t num_children;
    mpc_ast_t* ast;
} parser_tree;

void parser_init(parser* p);
void parser_dispose(parser* p);
int parser_parse(parser* p, char* input, parser_result* r);

parser_tree parser_result_get_tree(parser_result* r);
int parser_result_get_error(parser_result* r, char* buffer);
void parser_result_dispose_tree(parser_result* r);
void parser_result_dispose_error(parser_result* r);

parser_tree parser_tree_get_child(parser_tree* t, size_t index);

#endif  // PARSE_H_
