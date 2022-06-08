#ifndef ENV_H_
#define ENV_H_

#include "value.h"

struct environment {
    char** names;
    value** values;
    size_t length;
    size_t capacity;
    environment* parent;
};

void environment_init(environment* e);
void environment_dispose(environment* e);

value* environment_get(environment* e, char* name);
void environment_put(environment* e, char* name, value* v, int local);

void environment_register_number(environment* e, char* name, double number);
void environment_register_function(environment* e, char* name, value_fn function);
int environment_to_str(environment* e, char* buffer);

#endif  // ENV_H_
