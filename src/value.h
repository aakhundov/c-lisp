#ifndef VALUE_H_
#define VALUE_H_

#include <stdlib.h>

typedef enum {
    VALUE_NUMBER = 0,
    VALUE_SYMBOL = 1,
    VALUE_ERROR = 2,
    VALUE_INFO = 3,
    VALUE_STRING = 4,
    VALUE_BOOL = 5,
    VALUE_FUNCTION = 6,
    VALUE_SEXPR = 7,
    VALUE_QEXPR = 8
} value_type;

typedef struct value value;
typedef struct environment environment;

typedef value* (*value_fn)(value** args, size_t num_args, char* name, environment* env);

struct value {
    value_type type;
    double number;
    char* symbol;
    value_fn builtin;
    value* args;
    value* body;
    value** children;
    size_t num_children;
    size_t capacity;
};

value* value_new_number(double number);
value* value_new_symbol(char* symbol);
value* value_new_error(char* error, ...);
value* value_new_info(char* error, ...);
value* value_new_bool(int truth);
value* value_new_function(value* function);
value* value_new_function_builtin(value_fn builtin, char* symbol);
value* value_new_function_lambda(value* args, value* body);
value* value_new_sexpr();
value* value_new_qexpr();

value* value_parse(char* input);

void value_dispose(value* v);

value* value_copy(value* v);
value* value_compare(value* v1, value* v2);
value* value_equals(value* v1, value* v2);

void value_add_child(value* parent, value* child);
int value_to_str(value* v, char* buffer);
value* value_to_bool(value* v);

char* get_value_type_name(value_type t);

#endif  // VALUE_H_
