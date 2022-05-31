#include "eval.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "parse.h"
#include "value.h"

typedef value (*op_fn)(value*, size_t);

static value op_add(value* args, size_t num_args) {
    double result = (*args++).number;
    for (size_t i = 1; i < num_args; i++) {
        result += (*args++).number;
    }

    return value_new_number(result);
}

static value op_subtract(value* args, size_t num_args) {
    if (num_args == 1) {
        return value_new_number(-(*args).number);
    }

    double result = (*args++).number;
    for (size_t i = 1; i < num_args; i++) {
        result -= (*args++).number;
    }

    return value_new_number(result);
}

static value op_multiply(value* args, size_t num_args) {
    double result = (*args++).number;
    for (size_t i = 1; i < num_args; i++) {
        result *= (*args++).number;
    }

    return value_new_number(result);
}

static value op_divide(value* args, size_t num_args) {
    double result = (*args++).number;
    for (size_t i = 1; i < num_args; i++) {
        if ((*args).number == 0) {
            return value_new_error(ERROR_DIV_ZERO, NULL);
        }
        result /= (*args++).number;
    }

    return value_new_number(result);
}

static value op_modulo(value* args, size_t num_args) {
    int result = (int)(*args++).number;
    for (size_t i = 1; i < num_args; i++) {
        result %= (int)(*args++).number;
    }

    return value_new_number(result);
}

static value op_power(value* args, size_t num_args) {
    double result = (*args++).number;
    for (size_t i = 1; i < num_args; i++) {
        result = pow(result, (*args++).number);
    }

    return value_new_number(result);
}

static value op_minimum(value* args, size_t num_args) {
    double result = (*args++).number;
    for (size_t i = 1; i < num_args; i++) {
        double other = (*args++).number;
        if (other < result) {
            result = other;
        }
    }

    return value_new_number(result);
}

static value op_maximum(value* args, size_t num_args) {
    double result = (*args++).number;
    for (size_t i = 1; i < num_args; i++) {
        double other = (*args++).number;
        if (other > result) {
            result = other;
        }
    }

    return value_new_number(result);
}

static op_fn get_op_fn(char* op) {
    if (strcmp(op, "+") == 0 || strcmp(op, "add") == 0) {
        return op_add;
    } else if (strcmp(op, "-") == 0 || strcmp(op, "sub") == 0) {
        return op_subtract;
    } else if (strcmp(op, "*") == 0 || strcmp(op, "mul") == 0) {
        return op_multiply;
    } else if (strcmp(op, "/") == 0 || strcmp(op, "div") == 0) {
        return op_divide;
    } else if (strcmp(op, "%") == 0 || strcmp(op, "mod") == 0) {
        return op_modulo;
    } else if (strcmp(op, "^") == 0 || strcmp(op, "pow") == 0) {
        return op_power;
    } else if (strcmp(op, "min") == 0) {
        return op_minimum;
    } else if (strcmp(op, "max") == 0) {
        return op_maximum;
    } else {
        return NULL;
    }
}

value evaluate(tree* t) {
    if (strstr(t->tag, "number")) {
        errno = 0;
        double result = strtod(t->content, NULL);

        if (errno == 0) {
            return value_new_number(result);
        } else {
            return value_new_error(ERROR_BAD_NUMBER, t->content);
        }
    } else {
        size_t num_args = t->num_children - 3;
        value* args = malloc(num_args * sizeof(value));

        value* arg = args;
        for (size_t i = 0; i < num_args; i++) {
            tree child = tree_get_child(t, 2 + i);
            value v = evaluate(&child);

            if (v.type == VALUE_ERROR) {
                free(args);
                return v;
            }

            *arg++ = v;
        }

        tree op = tree_get_child(t, 1);
        op_fn fn = get_op_fn(op.content);

        value result;
        if (fn != NULL) {
            result = fn(args, num_args);
        } else {
            result = value_new_error(ERROR_BAD_OP, op.content);
        }

        free(args);

        return result;
    }
}
