#include "env.h"

#include <stdlib.h>
#include <string.h>

#include "value.h"

void environment_init(environment* e) {
    e->length = 0;
    e->capacity = 4;
    e->names = malloc(e->capacity * sizeof(char*));
    e->values = malloc(e->capacity * sizeof(value*));
}

void environment_dispose(environment* e) {
    for (size_t i = 0; i < e->length; i++) {
        free(e->names[i]);
        value_dispose(e->values[i]);
    }

    free(e->names);
    free(e->values);
}

static void environment_double(environment* e) {
    e->capacity *= 2;
    e->names = realloc(e->names, e->capacity * sizeof(char*));
    e->values = realloc(e->values, e->capacity * sizeof(value*));
}

value* environment_get(environment* e, char* name) {
    for (size_t i = 0; i < e->length; i++) {
        if (strcmp(e->names[i], name) == 0) {
            return value_copy(e->values[i]);
        }
    }

    return value_new_error("undefined symbol: %s", name);
}

void environment_put(environment* e, char* name, value* v) {
    for (size_t i = 0; i < e->length; i++) {
        if (strcmp(e->names[i], name) == 0) {
            value_dispose(e->values[i]);
            e->values[i] = value_copy(v);
            return;
        }
    }

    if (e->length == e->capacity) {
        environment_double(e);
    }

    e->names[e->length] = strdup(name);
    e->values[e->length] = value_copy(v);
    e->length++;
}

void environment_register_function(environment* e, char* name, value_fn function) {
    value* fn = value_new_function(function, name);
    environment_put(e, name, fn);
    value_dispose(fn);
}
