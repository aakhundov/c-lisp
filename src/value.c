#include "value.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parse.h"

value* value_new_number(double number) {
    value* v = malloc(sizeof(value));

    v->type = VALUE_NUMBER;
    v->number = number;

    return v;
}

value* value_new_symbol(char* symbol) {
    value* v = malloc(sizeof(value));

    v->type = VALUE_SYMBOL;
    v->symbol = malloc(strlen(symbol) + 1);
    strcpy(v->symbol, symbol);

    return v;
}

static value* value_new_symbol_from_args(char* format, va_list args) {
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);

    return value_new_symbol(buffer);
}

value* value_new_error(char* error, ...) {
    va_list args;
    va_start(args, error);
    value* v = value_new_symbol_from_args(error, args);
    va_end(args);

    v->type = VALUE_ERROR;

    return v;
}

value* value_new_info(char* info, ...) {
    va_list args;
    va_start(args, info);
    value* v = value_new_symbol_from_args(info, args);
    va_end(args);

    v->type = VALUE_INFO;

    return v;
}

value* value_new_bool(int truth) {
    value* v = malloc(sizeof(value));

    v->type = VALUE_BOOL;
    v->number = truth;

    return v;
}

value* value_new_function_builtin(value_fn builtin, char* symbol) {
    value* v = malloc(sizeof(value));

    v->type = VALUE_FUNCTION;
    v->builtin = builtin;
    v->symbol = malloc(strlen(symbol) + 1);
    v->args = NULL;
    v->body = NULL;

    strcpy(v->symbol, symbol);

    return v;
}

value* value_new_function_lambda(value* args, value* body) {
    assert(args->type == VALUE_QEXPR);
    assert(body->type == VALUE_QEXPR);

    value* v = malloc(sizeof(value));

    v->type = VALUE_FUNCTION;
    v->builtin = NULL;
    v->symbol = NULL;
    v->args = value_copy(args);
    v->body = value_copy(body);

    return v;
}

value* value_new_function(value* function) {
    assert(function->type == VALUE_FUNCTION);

    value* result = NULL;
    if (function->builtin != NULL) {
        result = value_new_function_builtin(function->builtin, function->symbol);
    } else {
        result = value_new_function_lambda(function->args, function->body);

        if (function->symbol != NULL) {
            result->symbol = malloc(strlen(function->symbol) + 1);
            strcpy(result->symbol, function->symbol);
        }
    }

    return result;
}

static value* value_new_expr(value_type type) {
    value* v = malloc(sizeof(value));

    v->type = type;
    v->num_children = 0;
    v->capacity = 4;
    v->children = malloc(v->capacity * sizeof(value*));

    return v;
}

value* value_new_sexpr() {
    return value_new_expr(VALUE_SEXPR);
}

value* value_new_qexpr() {
    return value_new_expr(VALUE_QEXPR);
}

void value_dispose(value* v) {
    switch (v->type) {
        case VALUE_NUMBER:
            break;
        case VALUE_SYMBOL:
        case VALUE_ERROR:
        case VALUE_INFO:
            free(v->symbol);
            break;
        case VALUE_BOOL:
            break;
        case VALUE_FUNCTION:
            if (v->builtin != NULL) {
                free(v->symbol);
            } else {
                value_dispose(v->args);
                value_dispose(v->body);
                if (v->symbol != NULL) {
                    free(v->symbol);
                }
            }
            break;
        case VALUE_SEXPR:
        case VALUE_QEXPR:
            for (size_t i = 0; i < v->num_children; i++) {
                if (v->children[i] != NULL) {
                    value_dispose(v->children[i]);
                }
            }
            free(v->children);
            break;
    }

    free(v);
}

void value_add_child(value* parent, value* child) {
    if (parent->num_children == parent->capacity) {
        parent->capacity *= 2;
        parent->children = realloc(
            parent->children,
            parent->capacity * sizeof(value*));
    }

    parent->children[parent->num_children] = child;
    parent->num_children++;
}

value* value_from_tree(tree* t) {
    if (strstr(t->tag, "number")) {
        errno = 0;
        double result = strtod(t->content, NULL);

        if (errno == 0) {
            return value_new_number(result);
        } else {
            return value_new_error("malformed number: %s", t->content);
        }
    } else if (strstr(t->tag, "symbol")) {
        return value_new_symbol(t->content);
    } else if (strstr(t->tag, "spec")) {
        if (strcmp(t->content, "#true") == 0) {
            return value_new_bool(1);
        } else if (strcmp(t->content, "#false") == 0) {
            return value_new_bool(0);
        } else if (strcmp(t->content, "#nil") == 0) {
            return value_new_qexpr();
        } else {
            return value_new_error("unknown special symbol: %s", t->content);
        }
    } else {
        value* expr = NULL;
        if (strcmp(t->tag, ">") == 0 || strstr(t->tag, "sexpr")) {
            expr = value_new_sexpr();
        } else if (strstr(t->tag, "qexpr")) {
            expr = value_new_qexpr();
        }

        for (size_t i = 0; i < t->num_children; i++) {
            tree child = tree_get_child(t, i);

            if (strcmp(child.content, "(") == 0 ||
                strcmp(child.content, ")") == 0 ||
                strcmp(child.content, "{") == 0 ||
                strcmp(child.content, "}") == 0 ||
                strcmp(child.tag, "regex") == 0) {
                continue;
            }

            value_add_child(expr, value_from_tree(&child));
        }

        return expr;
    }
}

value* value_copy(value* v) {
    value* result;

    switch (v->type) {
        case VALUE_NUMBER:
            result = value_new_number(v->number);
            break;
        case VALUE_SYMBOL:
            result = value_new_symbol(v->symbol);
            break;
        case VALUE_ERROR:
            result = value_new_error(v->symbol);
            break;
        case VALUE_INFO:
            result = value_new_info(v->symbol);
            break;
        case VALUE_BOOL:
            result = value_new_bool(v->number);
            break;
        case VALUE_FUNCTION:
            result = value_new_function(v);
            break;
        case VALUE_SEXPR:
        case VALUE_QEXPR:
            result = value_new_expr(v->type);
            for (size_t i = 0; i < v->num_children; i++) {
                value_add_child(result, value_copy(v->children[i]));
            }
            break;
        default:
            result = value_new_error("unknown value type: %d", v->type);
    }

    return result;
}

static int expr_to_str(value* v, char* buffer, char open, char close) {
    char* running = buffer;

    running += sprintf(running, "%c", open);
    for (size_t i = 0; i < v->num_children; i++) {
        running += value_to_str(v->children[i], running);
        if (i < v->num_children - 1) {
            running += sprintf(running, "%c", ' ');
        }
    }
    running += sprintf(running, "%c", close);
    *running = '\0';

    return running - buffer;
}

static int function_to_str(value* v, char* buffer) {
    if (v->builtin != NULL) {
        return sprintf(buffer, "<builtin %s>", v->symbol);
    } else {
        char args_buffer[1024];
        char body_buffer[1024];

        value_to_str(v->args, args_buffer);
        value_to_str(v->body, body_buffer);

        return sprintf(buffer, "<lambda %s %s>", args_buffer, body_buffer);
    }
}

int value_to_str(value* v, char* buffer) {
    switch (v->type) {
        case VALUE_NUMBER:
            return sprintf(buffer, "%g", v->number);
        case VALUE_SYMBOL:
            return sprintf(buffer, "%s", v->symbol);
        case VALUE_ERROR:
            return sprintf(buffer, "\x1B[31m%s\x1B[0m", v->symbol);
        case VALUE_INFO:
            return sprintf(buffer, "\x1B[32m%s\x1B[0m", v->symbol);
        case VALUE_BOOL:
            return sprintf(buffer, "%s", (v->number == 1) ? "#true" : "#false");
        case VALUE_FUNCTION:
            return function_to_str(v, buffer);
        case VALUE_SEXPR:
            return expr_to_str(v, buffer, '(', ')');
        case VALUE_QEXPR:
            return expr_to_str(v, buffer, '{', '}');
        default:
            return sprintf(buffer, "unknown value type: %d", v->type);
    }
}

value* value_compare(value* v1, value* v2) {
    value* result = NULL;

    if (v1->type != v2->type) {
        result = value_new_error(
            "can't compare values of different types: %s and %s",
            get_value_type_name(v1->type),
            get_value_type_name(v2->type));
    } else {
        value* sub_result;
        size_t min_children;

        switch (v1->type) {
            case VALUE_NUMBER:
                if (v1->number < v2->number) {
                    result = value_new_number(-1);
                } else if (v1->number > v2->number) {
                    result = value_new_number(1);
                } else {
                    result = value_new_number(0);
                }
                break;
            case VALUE_SYMBOL:
                result = value_new_number(strcmp(v1->symbol, v2->symbol));
                break;
            case VALUE_ERROR:
            case VALUE_INFO:
            case VALUE_BOOL:
            case VALUE_FUNCTION:
                result = value_new_error("incomprable type: %s", get_value_type_name(v1->type));
                break;
            case VALUE_SEXPR:
            case VALUE_QEXPR:
                min_children = v1->num_children;
                if (v2->num_children < v1->num_children) {
                    min_children = v2->num_children;
                }

                for (size_t i = 0; i < min_children; i++) {
                    sub_result = value_compare(v1->children[i], v2->children[i]);
                    if (sub_result->type == VALUE_ERROR || sub_result->number != 0) {
                        result = sub_result;
                        break;
                    } else {
                        value_dispose(sub_result);
                    }
                }

                if (result == NULL) {
                    result = value_new_number(v1->num_children - v2->num_children);
                }
                break;
            default:
                result = value_new_error("unknown value type: %d", v1->type);
        }
    }

    return result;
}

value* value_equals(value* v1, value* v2) {
    value* result = NULL;

    if (v1->type != v2->type) {
        result = value_new_bool(0);
    } else {
        value* sub_result;

        switch (v1->type) {
            case VALUE_NUMBER:
                result = value_new_bool(v1->number == v2->number ? 1 : 0);
                break;
            case VALUE_SYMBOL:
            case VALUE_ERROR:
            case VALUE_INFO:
                result = value_new_bool(strcmp(v1->symbol, v2->symbol) == 0 ? 1 : 0);
                break;
            case VALUE_BOOL:
                result = value_new_bool(v1->number == v2->number ? 1 : 0);
                break;
            case VALUE_FUNCTION:
                if (v1->builtin != NULL && v1->builtin != NULL) {
                    result = value_new_bool(v1->builtin == v2->builtin ? 1 : 0);
                } else if (v1->builtin == NULL && v1->builtin == NULL) {
                    sub_result = value_equals(v1->args, v2->args);
                    if (sub_result->type == VALUE_ERROR || sub_result->number == 0) {
                        result = sub_result;
                    } else {
                        value_dispose(sub_result);
                        sub_result = value_equals(v1->body, v2->body);
                        if (sub_result->type == VALUE_ERROR || sub_result->number == 0) {
                            result = sub_result;
                        } else {
                            value_dispose(sub_result);
                            result = value_new_bool(1);
                        }
                    }
                } else {
                    result = value_new_bool(0);
                }
                break;
            case VALUE_SEXPR:
            case VALUE_QEXPR:
                if (v1->num_children != v2->num_children) {
                    result = value_new_bool(0);
                } else {
                    for (size_t i = 0; i < v1->num_children; i++) {
                        sub_result = value_equals(v1->children[i], v2->children[i]);
                        if (sub_result->type == VALUE_ERROR || sub_result->number == 0) {
                            result = sub_result;
                            break;
                        } else {
                            value_dispose(sub_result);
                        }
                    }

                    if (result == NULL) {
                        result = value_new_bool(1);
                    }
                }
                break;
            default:
                result = value_new_error("unknown value type: %d", v1->type);
        }
    }

    return result;
}

char* get_value_type_name(value_type t) {
    switch (t) {
        case VALUE_NUMBER:
            return "number";
        case VALUE_SYMBOL:
            return "symbol";
        case VALUE_ERROR:
            return "error";
        case VALUE_INFO:
            return "info";
        case VALUE_BOOL:
            return "bool";
        case VALUE_FUNCTION:
            return "function";
        case VALUE_SEXPR:
            return "s-expr";
        case VALUE_QEXPR:
            return "q-expr";
        default:
            return "unknown";
    }
}
