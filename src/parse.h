#ifndef PARSE_H_
#define PARSE_H_

#include "mpc.h"

typedef struct {
    mpc_result_t res;
} parser_result;

typedef struct {
    char* tag;
    char* content;
    size_t num_children;
    mpc_ast_t* ast;
} parser_tree;

void parser_init();
void parser_dispose();

int parser_parse(char* input, parser_result* r);

parser_tree parser_result_get_tree(parser_result* r);
int parser_result_get_error(parser_result* r, char* buffer);
void parser_result_dispose_tree(parser_result* r);
void parser_result_dispose_error(parser_result* r);

parser_tree parser_tree_get_child(parser_tree* t, size_t index);

#endif  // PARSE_H_
