#ifndef PARSE_H_
#define PARSE_H_

#include "mpc.h"

typedef struct {
    mpc_parser_t* num;
    mpc_parser_t* op;
    mpc_parser_t* expr;
    mpc_parser_t* prog;
} parser;

typedef struct {
    mpc_result_t res;
} result;

void parser_init(parser* p);
void parser_dispose(parser* p);
int parser_parse(parser* p, char* input, result* r);

void result_print_tree(result* r);
void result_print_error(result* r);
void result_dispose_tree(result* r);
void result_dispose_error(result* r);

#endif  // PARSE_H_
