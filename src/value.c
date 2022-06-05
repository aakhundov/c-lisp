#include "value.h"

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

value* value_new_error(char* error, ...) {
    value* v = malloc(sizeof(value));
    char buffer[1024];

    va_list args;
    va_start(args, error);
    vsnprintf(buffer, sizeof(buffer), error, args);
    va_end(args);

    v->type = VALUE_ERROR;
    v->error = malloc(strlen(buffer) + 1);
    strcpy(v->error, buffer);

    return v;
}

value* value_new_symbol(char* symbol) {
    value* v = malloc(sizeof(value));

    v->type = VALUE_SYMBOL;
    v->symbol = malloc(strlen(symbol) + 1);
    strcpy(v->symbol, symbol);

    return v;
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
        case VALUE_ERROR:
            free(v->error);
            break;
        case VALUE_SYMBOL:
            free(v->symbol);
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
        case VALUE_ERROR:
            result = value_new_error(v->error);
            break;
        case VALUE_SYMBOL:
            result = value_new_symbol(v->symbol);
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

int value_to_str(value* v, char* buffer) {
    switch (v->type) {
        case VALUE_NUMBER:
            return sprintf(buffer, "%g", v->number);
        case VALUE_ERROR:
            return sprintf(buffer, "error: %s", v->error);
        case VALUE_SYMBOL:
            return sprintf(buffer, "%s", v->symbol);
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
        result = value_new_number(v1->type - v2->type);
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
            case VALUE_SYMBOL:
                result = value_new_number(strcmp(v1->symbol, v2->symbol));
            case VALUE_ERROR:
                result = value_new_number(strcmp(v1->error, v2->error));
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
            default:
                result = value_new_error("unknown value type: %d", v1->type);
        }
    }

    return result;
}
