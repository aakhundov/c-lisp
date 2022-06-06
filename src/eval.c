#include "eval.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "env.h"
#include "value.h"

#define ASSERT_NUM_ARGS(fn, num_args, expected_num_args) \
    {                                                    \
        if (num_args != expected_num_args) {             \
            return value_new_error(                      \
                "%s expects exactly %d arg%s, "          \
                "but got %d",                            \
                fn, expected_num_args,                   \
                (expected_num_args == 1 ? "" : "s"),     \
                num_args);                               \
        }                                                \
    }

#define ASSERT_MIN_NUM_ARGS(fn, num_args, min_num_args) \
    {                                                   \
        if (num_args < min_num_args) {                  \
            return value_new_error(                     \
                "%s expects at least %d arg%s, "        \
                "but got %d",                           \
                fn, min_num_args,                       \
                (min_num_args == 1 ? "" : "s"),         \
                num_args);                              \
        }                                               \
    }

#define ASSERT_ARG_TYPE(fn, arg, expected_type, ordinal) \
    {                                                    \
        if (arg->type != expected_type) {                \
            char buffer[1024];                           \
            value_to_str(arg, buffer);                   \
            return value_new_error(                      \
                "%s: arg #%d (%s) "                      \
                "must be of type %s, but got %s",        \
                fn, ordinal, buffer,                     \
                get_value_type_name(expected_type),      \
                get_value_type_name(arg->type));         \
        }                                                \
    }

#define ASSERT_ARGS_TYPE(fn, args, expected_type, num_args, offset)  \
    {                                                                \
        for (size_t i = 0; i < num_args; i++) {                      \
            ASSERT_ARG_TYPE(fn, args[i], expected_type, offset + i); \
        }                                                            \
    }

#define ASSERT_ARG_LENGTH(fn, arg, length, ordinal) \
    {                                               \
        if (arg->num_children != length) {          \
            char buffer[1024];                      \
            value_to_str(arg, buffer);              \
            return value_new_error(                 \
                "%s: arg #%d (%s) "                 \
                "must be %d-long",                  \
                fn, ordinal, buffer, length);       \
        }                                           \
    }

#define ASSERT_MIN_ARG_LENGTH(fn, arg, min_length, ordinal) \
    {                                                       \
        if (arg->num_children < min_length) {               \
            char buffer[1024];                              \
            value_to_str(arg, buffer);                      \
            return value_new_error(                         \
                "%s: arg #%d (%s) "                         \
                "must be at least %d-long, "                \
                "but got %d-long",                          \
                fn, ordinal, buffer, min_length,            \
                arg->num_children);                         \
        }                                                   \
    }

static value* builtin_add(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_MIN_NUM_ARGS(name, num_args, 1);
    ASSERT_ARGS_TYPE(name, args, VALUE_NUMBER, num_args, 0);

    double result = (*args++)->number;
    for (size_t i = 1; i < num_args; i++) {
        result += (*args++)->number;
    }

    return value_new_number(result);
}

static value* builtin_subtract(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_MIN_NUM_ARGS(name, num_args, 1);
    ASSERT_ARGS_TYPE(name, args, VALUE_NUMBER, num_args, 0);

    if (num_args == 1) {
        return value_new_number(-(*args)->number);
    }

    double result = (*args++)->number;
    for (size_t i = 1; i < num_args; i++) {
        result -= (*args++)->number;
    }

    return value_new_number(result);
}

static value* builtin_multiply(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_MIN_NUM_ARGS(name, num_args, 1);
    ASSERT_ARGS_TYPE(name, args, VALUE_NUMBER, num_args, 0);

    double result = (*args++)->number;
    for (size_t i = 1; i < num_args; i++) {
        result *= (*args++)->number;
    }

    return value_new_number(result);
}

static value* builtin_divide(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_MIN_NUM_ARGS(name, num_args, 1);
    ASSERT_ARGS_TYPE(name, args, VALUE_NUMBER, num_args, 0);

    double result = (*args++)->number;
    for (size_t i = 1; i < num_args; i++) {
        if ((*args)->number == 0) {
            return value_new_error("division by zero");
        }
        result /= (*args++)->number;
    }

    return value_new_number(result);
}

static value* builtin_modulo(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_MIN_NUM_ARGS(name, num_args, 1);
    ASSERT_ARGS_TYPE(name, args, VALUE_NUMBER, num_args, 0);

    int result = (int)(*args++)->number;
    for (size_t i = 1; i < num_args; i++) {
        result %= (int)(*args++)->number;
    }

    return value_new_number(result);
}

static value* builtin_power(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_MIN_NUM_ARGS(name, num_args, 1);
    ASSERT_ARGS_TYPE(name, args, VALUE_NUMBER, num_args, 0);

    double result = (*args++)->number;
    for (size_t i = 1; i < num_args; i++) {
        result = pow(result, (*args++)->number);
    }

    return value_new_number(result);
}

static value* builtin_minimum(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_MIN_NUM_ARGS(name, num_args, 1);
    ASSERT_ARGS_TYPE(name, args, VALUE_NUMBER, num_args, 0);

    double result = (*args++)->number;
    for (size_t i = 1; i < num_args; i++) {
        double other = (*args++)->number;
        if (other < result) {
            result = other;
        }
    }

    return value_new_number(result);
}

static value* builtin_maximum(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_MIN_NUM_ARGS(name, num_args, 1);
    ASSERT_ARGS_TYPE(name, args, VALUE_NUMBER, num_args, 0);

    double result = (*args++)->number;
    for (size_t i = 1; i < num_args; i++) {
        double other = (*args++)->number;
        if (other > result) {
            result = other;
        }
    }

    return value_new_number(result);
}

static value* builtin_list(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_MIN_NUM_ARGS(name, num_args, 1);

    value* result = value_new_qexpr();
    for (size_t i = 0; i < num_args; i++) {
        value_add_child(result, value_copy(args[i]));
    }

    return result;
}

static value* builtin_head(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_NUM_ARGS(name, num_args, 1);
    ASSERT_ARG_TYPE(name, args[0], VALUE_QEXPR, 0);
    ASSERT_MIN_ARG_LENGTH(name, args[0], 1, 0);

    value* result = value_new_qexpr();
    value_add_child(result, value_copy(args[0]->children[0]));

    return result;
}

static value* builtin_tail(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_NUM_ARGS(name, num_args, 1);
    ASSERT_ARG_TYPE(name, args[0], VALUE_QEXPR, 0);
    ASSERT_MIN_ARG_LENGTH(name, args[0], 1, 0);

    value* result = value_new_qexpr();
    for (size_t i = 1; i < args[0]->num_children; i++) {
        value_add_child(result, value_copy(args[0]->children[i]));
    }

    return result;
}

static value* builtin_join(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_MIN_NUM_ARGS(name, num_args, 1);
    ASSERT_ARGS_TYPE(name, args, VALUE_QEXPR, num_args, 0);

    value* result = value_new_qexpr();
    for (size_t i = 0; i < num_args; i++) {
        for (size_t j = 0; j < args[i]->num_children; j++) {
            value_add_child(result, value_copy(args[i]->children[j]));
        }
    }

    return result;
}

static value* builtin_eval(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_NUM_ARGS(name, num_args, 1);
    ASSERT_ARG_TYPE(name, args[0], VALUE_QEXPR, 0);

    value* temp = value_copy(args[0]);
    temp->type = VALUE_SEXPR;
    value* result = value_evaluate(temp, env);
    value_dispose(temp);

    return result;
}

static value* builtin_cons(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_NUM_ARGS(name, num_args, 2);
    ASSERT_ARG_TYPE(name, args[1], VALUE_QEXPR, 1);

    value* result = value_new_qexpr();
    value_add_child(result, value_copy(args[0]));
    for (size_t i = 0; i < args[1]->num_children; i++) {
        value_add_child(result, value_copy(args[1]->children[i]));
    }

    return result;
}

static value* builtin_len(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_NUM_ARGS(name, num_args, 1);
    ASSERT_ARG_TYPE(name, args[0], VALUE_QEXPR, 0);

    return value_new_number(args[0]->num_children);
}

static value* builtin_init(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_NUM_ARGS(name, num_args, 1);
    ASSERT_ARG_TYPE(name, args[0], VALUE_QEXPR, 0);
    ASSERT_MIN_ARG_LENGTH(name, args[0], 1, 0);

    value* result = value_new_qexpr();
    for (size_t i = 0; i < args[0]->num_children - 1; i++) {
        value_add_child(result, value_copy(args[0]->children[i]));
    }

    return result;
}

static value* builtin_def(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_MIN_NUM_ARGS(name, num_args, 2);
    ASSERT_ARG_TYPE(name, args[0], VALUE_QEXPR, 0);
    ASSERT_MIN_ARG_LENGTH(name, args[0], 1, 0);
    ASSERT_NUM_ARGS(name, num_args, args[0]->num_children + 1);

    for (size_t i = 0; i < args[0]->num_children; i++) {
        if (args[0]->children[i]->type != VALUE_SYMBOL) {
            return value_new_error("%s: first argument must consist of symbols.", name);
        }
    }

    for (size_t i = 0; i < args[0]->num_children; i++) {
        environment_put(env, args[0]->children[i]->symbol, args[i + 1]);
    }

    char buffer[1024];
    value_to_str(args[0], buffer);
    buffer[strlen(buffer) - 1] = '\0';

    return value_new_info("variables defined: %s", buffer + 1);
}

value* value_evaluate(value* v, environment* env) {
    if (v->type == VALUE_SEXPR) {
        value* result = NULL;
        value* temp = value_new_sexpr();

        for (size_t i = 0; i < v->num_children; i++) {
            value* evaled = value_evaluate(v->children[i], env);

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
            value* fn = temp->children[0];

            if (fn->type != VALUE_FUNCTION) {
                value_to_str(v, buffer);
                result = value_new_error("s-expr %s must start with a function", buffer);
            } else {
                result = fn->function(temp->children + 1, temp->num_children - 1, fn->symbol, env);
            }
        }

        value_dispose(temp);

        return result;
    } else if (v->type == VALUE_SYMBOL) {
        return environment_get(env, v->symbol);
    } else {
        return value_copy(v);
    }
}

void environment_register_builtins(environment* e) {
    // arithmetic builtins
    environment_register_function(e, "+", builtin_add);
    environment_register_function(e, "add", builtin_add);
    environment_register_function(e, "-", builtin_subtract);
    environment_register_function(e, "sub", builtin_subtract);
    environment_register_function(e, "*", builtin_multiply);
    environment_register_function(e, "mul", builtin_multiply);
    environment_register_function(e, "/", builtin_divide);
    environment_register_function(e, "div", builtin_divide);
    environment_register_function(e, "%", builtin_modulo);
    environment_register_function(e, "mod", builtin_modulo);
    environment_register_function(e, "^", builtin_power);
    environment_register_function(e, "pow", builtin_power);
    environment_register_function(e, "min", builtin_minimum);
    environment_register_function(e, "max", builtin_maximum);

    // list manipulation builtins
    environment_register_function(e, "list", builtin_list);
    environment_register_function(e, "head", builtin_head);
    environment_register_function(e, "tail", builtin_tail);
    environment_register_function(e, "join", builtin_join);
    environment_register_function(e, "eval", builtin_eval);
    environment_register_function(e, "cons", builtin_cons);
    environment_register_function(e, "len", builtin_len);
    environment_register_function(e, "init", builtin_init);

    // definition builtins
    environment_register_function(e, "def", builtin_def);
}
