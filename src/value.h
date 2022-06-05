#ifndef VALUE_H_
#define VALUE_H_

#include "parse.h"

typedef enum {
    VALUE_NUMBER = 0,
    VALUE_ERROR = 1,
    VALUE_SYMBOL = 2,
    VALUE_SEXPR = 3,
    VALUE_QEXPR = 4
} value_type;

typedef struct value value;

struct value {
    value_type type;
    double number;
    char* error;
    char* symbol;
    value** children;
    size_t num_children;
    size_t capacity;
};

value* value_new_number(double number);
value* value_new_error(char* error, ...);
value* value_new_symbol(char* symbol);
value* value_new_sexpr();
value* value_new_qexpr();

void value_dispose(value* v);

value* value_from_tree(tree* t);
value* value_copy(value* v);
value* value_compare(value* v1, value* v2);

void value_add_child(value* parent, value* child);
int value_to_str(value* v, char* buffer);

#endif  // VALUE_H_
