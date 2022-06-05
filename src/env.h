#ifndef ENV_H_
#define ENV_H_

#include "value.h"

struct environment {
    char** names;
    value** values;
    size_t length;
    size_t capacity;
};

void environment_init(environment* e);
void environment_dispose(environment* e);

value* environment_get(environment* e, char* name);
void environment_put(environment* e, char* name, value* v);

void environment_register_function(environment* e, char* name, value_fn function);

#endif  // ENV_H_
