#include "eval.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
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
                "must be exactly %d-long",          \
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

static value* builtin_fn(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_NUM_ARGS(name, num_args, 2);
    ASSERT_ARG_TYPE(name, args[0], VALUE_QEXPR, 0);
    ASSERT_ARG_TYPE(name, args[1], VALUE_QEXPR, 1);
    ASSERT_MIN_ARG_LENGTH(name, args[0], 1, 0);
    ASSERT_EXPR_CHILDREN_TYPE(name, args[0], VALUE_SYMBOL, 0);

    value* fn_args = value_new_qexpr();
    for (size_t i = 1; i < args[0]->num_children; i++) {
        value_add_child(fn_args, value_copy(args[0]->children[i]));
    }
    value* fn_body = value_copy(args[1]);

    value* lambda_args = value_new_qexpr();
    value_add_child(lambda_args, fn_args);
    value_add_child(lambda_args, fn_body);
    value* lambda = builtin_lambda(lambda_args->children, 2, name, env);
    value_dispose(lambda_args);

    if (lambda->type == VALUE_ERROR) {
        return lambda;
    } else {
        value* fn_name = value_new_qexpr();
        value_add_child(fn_name, value_copy(args[0]->children[0]));

        value* def_args = value_new_qexpr();
        value_add_child(def_args, fn_name);
        value_add_child(def_args, lambda);
        value* result = builtin_def(def_args->children, 2, name, env);
        value_dispose(def_args);

        return result;
    }
}

static value* builtin_eq(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_MIN_NUM_ARGS(name, num_args, 2);

    int truth = 1;
    int sub_truth;
    value* sub_result;

    for (size_t i = 0; i < num_args - 1; i++) {
        sub_result = value_equals(args[i], args[i + 1]);
        if (sub_result->type == VALUE_ERROR) {
            return sub_result;
        }
        sub_truth = sub_result->number;
        value_dispose(sub_result);

        if (sub_truth == 0) {
            truth = 0;
            break;
        }
    }

    return value_new_bool(truth);
}

static value* builtin_neq(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_MIN_NUM_ARGS(name, num_args, 2);

    int truth = 1;
    int sub_truth;
    value* sub_result;

    for (size_t i = 0; i < num_args - 1; i++) {
        for (size_t j = i + 1; j < num_args; j++) {
            sub_result = value_equals(args[i], args[j]);
            if (sub_result->type == VALUE_ERROR) {
                return sub_result;
            }
            sub_truth = sub_result->number;
            value_dispose(sub_result);

            if (sub_truth == 1) {
                truth = 0;
                break;
            }
        }
        if (truth == 0) {
            break;
        }
    }

    return value_new_bool(truth);
}

static value* builtin_comp(value** args, size_t num_args, char* name, environment* env, int direction, int inverse) {
    ASSERT_MIN_NUM_ARGS(name, num_args, 2);

    int truth = 1;
    int sub_direction;
    value* sub_result;

    for (size_t i = 0; i < num_args - 1; i++) {
        sub_result = value_compare(args[i], args[i + 1]);
        if (sub_result->type == VALUE_ERROR) {
            return sub_result;
        }
        sub_direction = sub_result->number * direction;
        value_dispose(sub_result);

        if ((!inverse && sub_direction <= 0) || (inverse && sub_direction > 0)) {
            truth = 0;
            break;
        }
    }

    return value_new_bool(truth);
}

static value* builtin_gt(value** args, size_t num_args, char* name, environment* env) {
    return builtin_comp(args, num_args, name, env, 1, 0);
}

static value* builtin_gte(value** args, size_t num_args, char* name, environment* env) {
    return builtin_comp(args, num_args, name, env, -1, 1);
}

static value* builtin_lt(value** args, size_t num_args, char* name, environment* env) {
    return builtin_comp(args, num_args, name, env, -1, 0);
}

static value* builtin_lte(value** args, size_t num_args, char* name, environment* env) {
    return builtin_comp(args, num_args, name, env, 1, 1);
}

static value* builtin_null_q(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_NUM_ARGS(name, num_args, 1);

    value* result = value_to_bool(args[0]);
    result->number = 1 - result->number;

    return result;
}

static value* builtin_zero_q(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_NUM_ARGS(name, num_args, 1);
    ASSERT_ARG_TYPE(name, args[0], VALUE_NUMBER, 0);

    return value_new_bool((args[0]->number == 0) ? 1 : 0);
}

static value* builtin_list_q(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_NUM_ARGS(name, num_args, 1);

    return value_new_bool((args[0]->type == VALUE_QEXPR) ? 1 : 0);
}

static value* builtin_if(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_NUM_ARGS(name, num_args, 3);
    ASSERT_ARG_TYPE(name, args[1], VALUE_QEXPR, 1);
    ASSERT_ARG_TYPE(name, args[2], VALUE_QEXPR, 2);

    value* truth = value_to_bool(args[0]);
    if (truth->type == VALUE_ERROR) {
        return truth;
    } else if (truth->number == 1) {
        value_dispose(truth);
        return builtin_eval(&args[1], 1, name, env);
    } else {
        value_dispose(truth);
        return builtin_eval(&args[2], 1, name, env);
    }
}

static value* builtin_cond(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_MIN_NUM_ARGS(name, num_args, 2);

    if (num_args % 2 != 0) {
        return value_new_error("%s expects an even number of args", name);
    }

    for (size_t i = 0; i < num_args / 2; i++) {
        ASSERT_ARG_TYPE(name, args[2 * i + 1], VALUE_QEXPR, 2 * i + 1);
    }

    for (size_t i = 0; i < num_args / 2; i++) {
        value* evaled = value_evaluate(args[2 * i], env);
        if (evaled->type == VALUE_ERROR) {
            return evaled;
        }

        value* truth = value_to_bool(evaled);
        value_dispose(evaled);
        if (truth->type == VALUE_ERROR) {
            return truth;
        } else if (truth->number == 1) {
            value_dispose(truth);
            return builtin_eval(&args[2 * i + 1], 1, name, env);
        } else {
            value_dispose(truth);
        }
    }

    // no condition was hit
    return value_new_qexpr();
}

static value* builtin_logical(value** args, size_t num_args, char* name, environment* env, int short_circuit) {
    for (size_t i = 0; i < num_args; i++) {
        value* evaled = value_evaluate(args[i], env);
        if (evaled->type == VALUE_ERROR) {
            return evaled;
        }

        value* truth = value_to_bool(evaled);
        value_dispose(evaled);
        if (truth->type == VALUE_ERROR || truth->number == short_circuit) {
            return truth;
        }
        value_dispose(truth);
    }

    return value_new_bool(1 - short_circuit);
}

static value* builtin_and(value** args, size_t num_args, char* name, environment* env) {
    return builtin_logical(args, num_args, name, env, 0);
}

static value* builtin_or(value** args, size_t num_args, char* name, environment* env) {
    return builtin_logical(args, num_args, name, env, 1);
}

static value* builtin_not(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_NUM_ARGS(name, num_args, 1);

    value* truth = value_to_bool(args[0]);
    if (truth->type == VALUE_BOOL) {
        truth->number = 1 - truth->number;
    }

    return truth;
}

static value* builtin_del(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_NUM_ARGS(name, num_args, 1);
    ASSERT_ARG_TYPE(name, args[0], VALUE_QEXPR, 0);
    ASSERT_ARG_LENGTH(name, args[0], 1, 0);
    ASSERT_EXPR_CHILDREN_TYPE(name, args[0], VALUE_SYMBOL, 0);

    char* item_name = args[0]->children[0]->symbol;
    if (environment_delete(env, item_name) == 1) {
        return value_new_info("deleted: %s", item_name);
    } else {
        return value_new_error("not found: %s", item_name);
    }
}

static value* builtin_seval(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_MIN_NUM_ARGS(name, num_args, 1);
    ASSERT_ARG_TYPE(name, args[0], VALUE_STRING, 0);

    int batch = 0;
    int verbose = 1;
    if (num_args > 1) {
        ASSERT_ARG_TYPE(name, args[1], VALUE_BOOL, 1);
        batch = args[1]->number;
        if (num_args > 2) {
            ASSERT_ARG_TYPE(name, args[2], VALUE_BOOL, 2);
            verbose = args[2]->number;
        }
    }

    value* v = value_parse(args[0]->symbol);

    if (v->type != VALUE_ERROR) {
        if (batch) {
            char buffer[1024];
            size_t counter = 0;
            size_t num_children = v->num_children;
            for (size_t i = 0; i < num_children; i++) {
                value* e = value_evaluate(v->children[i], env);
                if (verbose) {
                    value_to_str(e, buffer);
                    printf("\x1B[32m%zu:\x1B[0m %s\n", ++counter, buffer);
                }
                value_dispose(e);
            }
            value_dispose(v);
            v = value_new_info(
                "evaluated %zu expression%s",
                num_children, (num_children > 1 ? "s" : ""));
        } else {
            value* e = value_evaluate(v, env);
            value_dispose(v);
            v = e;
        }
    }

    return v;
}

static value* builtin_load(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_NUM_ARGS(name, num_args, 1);
    ASSERT_ARG_TYPE(name, args[0], VALUE_STRING, 0);

    value* result = NULL;

    char content[65536];
    FILE* file = fopen(args[0]->symbol, "r");
    if (file) {
        int read = fread(content, 1, sizeof(content), file);
        content[read] = '\0';  // guard against the old call stack
        if (ferror(file)) {
            result = value_new_error("error reading from file: %s", args[0]->symbol);
        } else {
            value* seval_args = value_new_sexpr();
            value_add_child(seval_args, value_new_string(content));
            value_add_child(seval_args, value_new_bool(1));  // batch
            value_add_child(seval_args, value_new_bool(1));  // verbose

            result = builtin_seval(seval_args->children, 3, name, env);

            value_dispose(seval_args);
        }
        fclose(file);
    } else {
        result = value_new_error("failed to open file: %s", args[0]->symbol);
    }

    return result;
}

static value* builtin_print(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_MIN_NUM_ARGS(name, num_args, 1);

    char buffer[1024];
    for (size_t i = 0; i < num_args; i++) {
        value_to_str(args[i], buffer);
        printf("%s", buffer);
        if (i < num_args - 1) {
            printf(" ");
        }
    }
    printf("\n");

    return value_new_sexpr();
}

static value* builtin_error(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_NUM_ARGS(name, num_args, 1);
    ASSERT_ARG_TYPE(name, args[0], VALUE_STRING, 0);

    return value_new_error(args[0]->symbol);
}

static value* builtin_info(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_NUM_ARGS(name, num_args, 1);
    ASSERT_ARG_TYPE(name, args[0], VALUE_STRING, 0);

    return value_new_info(args[0]->symbol);
}

static value* builtin_sjoin(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_MIN_NUM_ARGS(name, num_args, 1);
    ASSERT_ARGS_TYPE(name, args, VALUE_STRING, num_args, 0);

    size_t dest_len = 0;
    for (size_t i = 0; i < num_args; i++) {
        dest_len += strlen(args[i]->symbol);
    }

    char* dest = malloc(dest_len + 1);
    char* running = dest;
    for (size_t i = 0; i < num_args; i++) {
        running += sprintf(running, "%s", args[i]->symbol);
    }
    value* result = value_new_string(dest);
    free(dest);

    return result;
}

static value* builtin_shead(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_NUM_ARGS(name, num_args, 1);
    ASSERT_ARG_TYPE(name, args[0], VALUE_STRING, 0);

    size_t len = strlen(args[0]->symbol);
    if (len < 1) {
        return value_new_error(
            "%s: arg #%d must be at least %d-long, but got %d-long",
            name, 0, 1, 0);
    }

    char dest[2];
    snprintf(dest, 2, "%s", args[0]->symbol);

    return value_new_string(dest);
}

static value* builtin_stail(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_NUM_ARGS(name, num_args, 1);
    ASSERT_ARG_TYPE(name, args[0], VALUE_STRING, 0);

    size_t len = strlen(args[0]->symbol);
    if (len < 1) {
        return value_new_error(
            "%s: arg #%d must be at least %d-long, but got %d-long",
            name, 0, 1, 0);
    }

    return value_new_string(args[0]->symbol + 1);
}

static value* builtin_sinit(value** args, size_t num_args, char* name, environment* env) {
    ASSERT_NUM_ARGS(name, num_args, 1);
    ASSERT_ARG_TYPE(name, args[0], VALUE_STRING, 0);

    size_t len = strlen(args[0]->symbol);
    if (len < 1) {
        return value_new_error(
            "%s: arg #%d must be at least %d-long, but got %d-long",
            name, 0, 1, 0);
    }

    char* dest = malloc(len);
    snprintf(dest, len, "%s", args[0]->symbol);
    value* result = value_new_string(dest);
    free(dest);

    return result;
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

static int is_delayed_evaluation_function(value* fn) {
    assert(fn->type == VALUE_FUNCTION);

    if (fn->builtin == builtin_and ||
        fn->builtin == builtin_or ||
        fn->builtin == builtin_cond) {
        return 1;
    } else {
        return 0;
    }
}

value* value_evaluate(value* v, environment* env) {
    if (v->type == VALUE_SEXPR) {
        char buffer[1024];
        value* result = NULL;

        value* temp = value_new_sexpr();
        if (v->num_children == 0) {
            return temp;
        }

        value* child = NULL;
        if (result == NULL) {
            child = value_evaluate(v->children[0], env);
            if (child->type == VALUE_ERROR) {
                result = child;
            } else {
                value_add_child(temp, child);
            }
        }

        if (result == NULL) {
            if (v->num_children == 1) {
                result = temp->children[0];
                temp->children[0] = NULL;  // don't dispose
            }
        }

        value* fn = NULL;
        if (result == NULL) {
            fn = temp->children[0];
            if (fn->type != VALUE_FUNCTION) {
                value_to_str(v, buffer);
                result = value_new_error("s-expr %s must start with a function", buffer);
            }
        }

        if (result == NULL) {
            for (size_t i = 1; i < v->num_children; i++) {
                if (is_delayed_evaluation_function(fn)) {
                    child = value_copy(v->children[i]);
                } else {
                    child = value_evaluate(v->children[i], env);
                }

                if (child->type == VALUE_ERROR) {
                    result = child;
                    break;
                } else {
                    value_add_child(temp, child);
                }
            }
        }

        if (result == NULL) {
            if (fn->builtin != NULL) {
                result = fn->builtin(temp->children + 1, temp->num_children - 1, fn->symbol, env);
            } else {
                result = call_lambda(fn, temp->children + 1, temp->num_children - 1, env);
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
    environment_register_function(e, "fn", builtin_fn);
    environment_register_function(e, "del", builtin_del);

    // comparison functions
    environment_register_function(e, "==", builtin_eq);
    environment_register_function(e, "eq", builtin_eq);
    environment_register_function(e, "!=", builtin_neq);
    environment_register_function(e, "neq", builtin_neq);
    environment_register_function(e, ">", builtin_gt);
    environment_register_function(e, "gt", builtin_gt);
    environment_register_function(e, ">=", builtin_gte);
    environment_register_function(e, "gte", builtin_gte);
    environment_register_function(e, "<", builtin_lt);
    environment_register_function(e, "lt", builtin_lt);
    environment_register_function(e, "<=", builtin_lte);
    environment_register_function(e, "lte", builtin_lte);
    environment_register_function(e, "null?", builtin_null_q);
    environment_register_function(e, "empty?", builtin_null_q);
    environment_register_function(e, "zero?", builtin_zero_q);
    environment_register_function(e, "list?", builtin_list_q);

    // conditional functions
    environment_register_function(e, "if", builtin_if);
    environment_register_function(e, "cond", builtin_cond);
    environment_register_function(e, "switch", builtin_cond);

    // logical functions
    environment_register_function(e, "&&", builtin_and);
    environment_register_function(e, "and", builtin_and);
    environment_register_function(e, "||", builtin_or);
    environment_register_function(e, "or", builtin_or);
    environment_register_function(e, "!", builtin_not);
    environment_register_function(e, "not", builtin_not);

    // string functions
    environment_register_function(e, "seval", builtin_seval);
    environment_register_function(e, "load", builtin_load);
    environment_register_function(e, "print", builtin_print);
    environment_register_function(e, "error", builtin_error);
    environment_register_function(e, "info", builtin_info);
    environment_register_function(e, "sjoin", builtin_sjoin);
    environment_register_function(e, "shead", builtin_shead);
    environment_register_function(e, "stail", builtin_stail);
    environment_register_function(e, "sinit", builtin_sinit);
}
