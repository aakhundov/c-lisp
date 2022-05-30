#include "eval.h"

#include <stdlib.h>
#include <string.h>

#include "parse.h"

typedef double (*op_fn)(double*, size_t);

static double add(double* args, size_t num_args) {
    double result = *args++;
    for (size_t i = 1; i < num_args; i++) {
        result += *args++;
    }

    return result;
}

static double subtract(double* args, size_t num_args) {
    if (num_args == 1) {
        return -(*args);
    }

    double result = *args++;
    for (size_t i = 1; i < num_args; i++) {
        result -= *args++;
    }

    return result;
}

static double multiply(double* args, size_t num_args) {
    double result = *args++;
    for (size_t i = 1; i < num_args; i++) {
        result *= *args++;
    }

    return result;
}

static double divide(double* args, size_t num_args) {
    double result = *args++;
    for (size_t i = 1; i < num_args; i++) {
        result /= *args++;
    }

    return result;
}

static double modulo(double* args, size_t num_args) {
    int result = (int)(*args++);
    for (size_t i = 1; i < num_args; i++) {
        result %= (int)(*args++);
    }

    return result;
}

static op_fn get_op_fn(char* op) {
    if (strcmp(op, "+") == 0 || strcmp(op, "add") == 0) {
        return add;
    } else if (strcmp(op, "-") == 0 || strcmp(op, "sub") == 0) {
        return subtract;
    } else if (strcmp(op, "*") == 0 || strcmp(op, "mul") == 0) {
        return multiply;
    } else if (strcmp(op, "/") == 0 || strcmp(op, "div") == 0) {
        return divide;
    } else if (strcmp(op, "%") == 0 || strcmp(op, "mod") == 0) {
        return modulo;
    } else {
        // default
        return add;
    }
}

double evaluate(tree* t) {
    if (strstr(t->tag, "number")) {
        return atof(t->content);
    } else {
        size_t num_args = t->num_children - 3;
        double* args = malloc(num_args * sizeof(double));

        double* arg = args;
        for (size_t i = 0; i < num_args; i++) {
            tree child = tree_get_child(t, 2 + i);
            *arg++ = evaluate(&child);
        }

        tree op = tree_get_child(t, 1);
        op_fn fn = get_op_fn(op.content);
        double result = fn(args, num_args);
        free(args);

        return result;
    }
}
