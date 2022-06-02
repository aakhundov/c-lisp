#include "eval.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "value.h"

#define ASSERT_NUM_ARGS(op, num_args, expected_num_args) \
    {                                                    \
        if (num_args != expected) {                      \
            return value_new_error(                      \
                "'%s' expects %d args",                  \
                op, expected_num_args);                  \
        }                                                \
    }

#define ASSERT_MIN_NUM_ARGS(op, num_args, min_num_args) \
    {                                                   \
        if (num_args < min_num_args) {                  \
            return value_new_error(                     \
                "'%s' expects at least %d args",        \
                op, min_num_args);                      \
        }                                               \
    }

#define ASSERT_ARG_TYPE(op, arg, expected_type, ordinal) \
    {                                                    \
        if (arg->type != expected_type) {                \
            return value_new_error(                      \
                "'%s': wrong type of arg #%d",           \
                op, ordinal);                            \
        }                                                \
    }

#define ASSERT_ARGS_TYPE(op, args, expected_type, num_args, offset)  \
    {                                                                \
        for (size_t i = 0; i < num_args; i++) {                      \
            ASSERT_ARG_TYPE(op, args[i], expected_type, offset + i); \
        }                                                            \
    }

#define ASSERT_ARG_LEN(op, arg, len, ordinal)    \
    {                                            \
        if (arg->num_children != len) {          \
            return value_new_error(              \
                "'%s': arg #%d must be %d-long", \
                op, ordinal, len);               \
        }                                        \
    }

#define ASSERT_MIN_ARG_LEN(op, arg, min_len, ordinal)     \
    {                                                     \
        if (arg->num_children < min_len) {                \
            return value_new_error(                       \
                "'%s': arg #%d must be at least %d-long", \
                op, ordinal, min_len);                    \
        }                                                 \
    }

typedef value* (*op_fn)(value**, size_t, char* op);

static value* op_add(value** args, size_t num_args, char* op) {
    ASSERT_MIN_NUM_ARGS(op, num_args, 1);
    ASSERT_ARGS_TYPE(op, args, VALUE_NUMBER, num_args, 0);

    double result = (*args++)->number;
    for (size_t i = 1; i < num_args; i++) {
        result += (*args++)->number;
    }

    return value_new_number(result);
}

static value* op_subtract(value** args, size_t num_args, char* op) {
    ASSERT_MIN_NUM_ARGS(op, num_args, 1);
    ASSERT_ARGS_TYPE(op, args, VALUE_NUMBER, num_args, 0);

    if (num_args == 1) {
        return value_new_number(-(*args)->number);
    }

    double result = (*args++)->number;
    for (size_t i = 1; i < num_args; i++) {
        result -= (*args++)->number;
    }

    return value_new_number(result);
}

static value* op_multiply(value** args, size_t num_args, char* op) {
    ASSERT_MIN_NUM_ARGS(op, num_args, 1);
    ASSERT_ARGS_TYPE(op, args, VALUE_NUMBER, num_args, 0);

    double result = (*args++)->number;
    for (size_t i = 1; i < num_args; i++) {
        result *= (*args++)->number;
    }

    return value_new_number(result);
}

static value* op_divide(value** args, size_t num_args, char* op) {
    ASSERT_MIN_NUM_ARGS(op, num_args, 1);
    ASSERT_ARGS_TYPE(op, args, VALUE_NUMBER, num_args, 0);

    double result = (*args++)->number;
    for (size_t i = 1; i < num_args; i++) {
        if ((*args)->number == 0) {
            return value_new_error("division by zero");
        }
        result /= (*args++)->number;
    }

    return value_new_number(result);
}

static value* op_modulo(value** args, size_t num_args, char* op) {
    ASSERT_MIN_NUM_ARGS(op, num_args, 1);
    ASSERT_ARGS_TYPE(op, args, VALUE_NUMBER, num_args, 0);

    int result = (int)(*args++)->number;
    for (size_t i = 1; i < num_args; i++) {
        result %= (int)(*args++)->number;
    }

    return value_new_number(result);
}

static value* op_power(value** args, size_t num_args, char* op) {
    ASSERT_MIN_NUM_ARGS(op, num_args, 1);
    ASSERT_ARGS_TYPE(op, args, VALUE_NUMBER, num_args, 0);

    double result = (*args++)->number;
    for (size_t i = 1; i < num_args; i++) {
        result = pow(result, (*args++)->number);
    }

    return value_new_number(result);
}

static value* op_minimum(value** args, size_t num_args, char* op) {
    ASSERT_MIN_NUM_ARGS(op, num_args, 1);
    ASSERT_ARGS_TYPE(op, args, VALUE_NUMBER, num_args, 0);

    double result = (*args++)->number;
    for (size_t i = 1; i < num_args; i++) {
        double other = (*args++)->number;
        if (other < result) {
            result = other;
        }
    }

    return value_new_number(result);
}

static value* op_maximum(value** args, size_t num_args, char* op) {
    ASSERT_MIN_NUM_ARGS(op, num_args, 1);
    ASSERT_ARGS_TYPE(op, args, VALUE_NUMBER, num_args, 0);

    double result = (*args++)->number;
    for (size_t i = 1; i < num_args; i++) {
        double other = (*args++)->number;
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

value* value_evaluate(value* v) {
    if (v->type == VALUE_SEXPR) {
        value* result = NULL;
        value* temp = value_new_sexpr();

        for (size_t i = 0; i < v->num_children; i++) {
            value* evaled = value_evaluate(v->children[i]);

            if (evaled->type == VALUE_ERROR) {
                result = evaled;
                break;
            } else {
                value_add_child(temp, evaled);
            }
        }

        if (result == NULL) {
            if (temp->num_children == 0) {
                return temp;
            } else if (temp->num_children == 1) {
                result = temp->children[0];
                temp->children[0] = NULL;
            }
        }

        if (result == NULL) {
            char buffer[1024];
            value* op_value = temp->children[0];

            if (op_value->type != VALUE_SYMBOL) {
                value_to_str(v, buffer);
                result = value_new_error("s-expr doesn't start with a symbol: '%s'", buffer);
            } else {
                op_fn op = get_op_fn(op_value->symbol);

                if (op == NULL) {
                    result = value_new_error("unrecognizer operator: '%s'", op_value->symbol);
                } else {
                    result = op(temp->children + 1, temp->num_children - 1, op_value->symbol);
                }
            }
        }

        value_dispose(temp);

        return result;
    } else {
        return value_copy(v);
    }
}
