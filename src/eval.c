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

#define ASSERT_EXPR_CHILDREN_TYPE(fn, arg, expected_type, ordinal) \
    {                                                              \
        for (size_t i = 0; i < arg->num_children; i++) {           \
            if (arg->children[i]->type != expected_type) {         \
                char buffer[1024];                                 \
                value_to_str(arg, buffer);                         \
                return value_new_error(                            \
                    "%s: arg #%d (%s) "                            \
                    "must consist of %s children, but got %s",     \
                    fn, ordinal, buffer,                           \
                    get_value_type_name(expected_type),            \
                    get_value_type_name(arg->children[i]->type));  \
            }                                                      \
        }                                                          \
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

static value* builtin_first(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_NUM_ARGS(name, num_args, 1);
    ASSERT_ARG_TYPE(name, args[0], VALUE_QEXPR, 0);
    ASSERT_MIN_ARG_LENGTH(name, args[0], 1, 0);

    return value_copy(args[0]->children[0]);
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

static value* builtin_var(value** args, size_t num_args, char* name, environment* env, int local) {
    ASSERT_MIN_NUM_ARGS(name, num_args, 2);
    ASSERT_ARG_TYPE(name, args[0], VALUE_QEXPR, 0);
    ASSERT_MIN_ARG_LENGTH(name, args[0], 1, 0);
    ASSERT_NUM_ARGS(name, num_args, args[0]->num_children + 1);
    ASSERT_EXPR_CHILDREN_TYPE(name, args[0], VALUE_SYMBOL, 0);

    for (size_t i = 0; i < args[0]->num_children; i++) {
        if (args[i + 1]->type == VALUE_FUNCTION) {
            if (args[i + 1]->symbol != NULL) {
                free(args[i + 1]->symbol);
            }

            // add symbol to the defined (lambda) function on the fly
            args[i + 1]->symbol = malloc(strlen(args[0]->children[i]->symbol) + 1);
            strcpy(args[i + 1]->symbol, args[0]->children[i]->symbol);
        }

        environment_put(env, args[0]->children[i]->symbol, args[i + 1], local);
    }

    char buffer[1024];
    value_to_str(args[0], buffer);
    buffer[strlen(buffer) - 1] = '\0';

    return value_new_info("defined: %s", buffer + 1);
}

static value* builtin_def(value** args, size_t num_args, char* name, environment* env) {
    return builtin_var(args, num_args, name, env, 0);
}

static value* builtin_local(value** args, size_t num_args, char* name, environment* env) {
    return builtin_var(args, num_args, name, env, 1);
}

static value* builtin_lambda(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_NUM_ARGS(name, num_args, 2);
    ASSERT_ARG_TYPE(name, args[0], VALUE_QEXPR, 0);
    ASSERT_ARG_TYPE(name, args[1], VALUE_QEXPR, 1);
    ASSERT_EXPR_CHILDREN_TYPE(name, args[0], VALUE_SYMBOL, 0);

    if (args[0]->num_children > 1 && strcmp(args[0]->children[1]->symbol, "&") == 0) {
        if (args[0]->num_children != 3) {
            return value_new_error("exactly one argument must follow &");
        }
    }

    return value_new_function_lambda(args[0], args[1]);
}

static value* call_lambda(value* lambda, value** args, size_t num_args, environment* env) {
    char* name = (lambda->symbol != NULL) ? lambda->symbol : "lambda";

    if (lambda->args->num_children > 1 && strcmp(lambda->args->children[1]->symbol, "&") == 0) {
        ASSERT_MIN_NUM_ARGS(name, num_args, 1);
    } else {
        ASSERT_NUM_ARGS(name, num_args, lambda->args->num_children);
    }

    environment local;
    environment_init(&local);
    local.parent = env;

    if (lambda->args->num_children > 1 && strcmp(lambda->args->children[1]->symbol, "&") == 0) {
        value* rest = value_new_qexpr();
        for (size_t i = 1; i < num_args; i++) {
            value_add_child(rest, value_copy(args[i]));
        }

        environment_put(&local, lambda->args->children[0]->symbol, args[0], 1);
        environment_put(&local, lambda->args->children[2]->symbol, rest, 1);

        value_dispose(rest);
    } else {
        for (size_t i = 0; i < lambda->args->num_children; i++) {
            environment_put(&local, lambda->args->children[i]->symbol, args[i], 1);
        }
    }

    value* result = builtin_eval(&lambda->body, 1, name, &local);

    environment_dispose(&local);

    return result;
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
                if (fn->builtin != NULL) {
                    result = fn->builtin(temp->children + 1, temp->num_children - 1, fn->symbol, env);
                } else {
                    result = call_lambda(fn, temp->children + 1, temp->num_children - 1, env);
                }
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
    // constants
    environment_register_number(e, "E", 2.7182818);
    environment_register_number(e, "PI", 3.1415926);

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
    environment_register_function(e, "first", builtin_first);
    environment_register_function(e, "car", builtin_first);
    environment_register_function(e, "head", builtin_head);
    environment_register_function(e, "tail", builtin_tail);
    environment_register_function(e, "cdr", builtin_tail);
    environment_register_function(e, "join", builtin_join);
    environment_register_function(e, "eval", builtin_eval);
    environment_register_function(e, "cons", builtin_cons);
    environment_register_function(e, "len", builtin_len);
    environment_register_function(e, "init", builtin_init);

    // definition builtins
    environment_register_function(e, "def", builtin_def);
    environment_register_function(e, "local", builtin_local);
    environment_register_function(e, "lambda", builtin_lambda);
}
