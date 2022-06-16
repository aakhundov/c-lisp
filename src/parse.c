#include "parse.h"

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "str.h"
#include "value.h"

static const char* exp_chars = "eE";
static const char* sign_chars = "+-";
static const char* digit_chars = "0123456789";
static const char* whitespace_chars = " \t\r\n\v";
static const char* alpha_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";
static const char* symbol_chars =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "_+-*/%^\\=<>!&|?.";

static value* create_parsing_error(size_t offset, char* format, ...) {
    char extended_format[128];
    snprintf(
        extended_format, sizeof(extended_format),
        "parsing error at %zu: %s", offset, format);

    va_list args;
    va_start(args, format);
    value* error = value_new_error_from_args(extended_format, args);
    va_end(args);

    return error;
}

static int is_number(char* symbol) {
    char* running = symbol;

    int digit_seen = 0;
    int exp_seen = 0;
    int dot_seen = 0;

    while (*running != '\0') {
        if (strchr(digit_chars, *running)) {
            digit_seen = 1;
        } else if (strchr(sign_chars, *running)) {
            if (running != symbol && !strchr(exp_chars, *(running - 1))) {
                return 0;
            }
        } else if (strchr(exp_chars, *running)) {
            if (exp_seen || !digit_seen) {
                return 0;
            }
            digit_seen = 0;
            exp_seen = 1;
        } else if (*running == '.') {
            if (dot_seen || exp_seen) {
                return 0;
            }
            dot_seen = 1;
        } else {
            return 0;
        }
        running++;
    }

    return digit_seen;
}

static value* value_read_number(char* content, size_t offset) {
    errno = 0;
    double result = strtod(content, NULL);

    if (errno == 0) {
        return value_new_number(result);
    } else {
        return create_parsing_error(offset, "malformed number: %s", content);
    }
}

static value* value_read_special(char* content, size_t offset) {
    value* result = NULL;

    size_t length = strlen(content);
    char* lower = malloc(length + 1);
    for (size_t i = 0; i < length; i++) {
        lower[i] = tolower(content[i]);
    }
    lower[length] = '\0';

    if (strcmp(lower, "#true") == 0) {
        result = value_new_bool(1);
    } else if (strcmp(lower, "#false") == 0) {
        result = value_new_bool(0);
    } else if (strcmp(lower, "#null") == 0) {
        result = value_new_qexpr();
    } else {
        result = create_parsing_error(offset, "unknown special symbol: %s", content);
    }

    free(lower);

    return result;
}

static value* value_read_string(char* content, size_t offset) {
    size_t length = strlen(content);
    content[length - 1] = '\0';  // remove trailing quote
    content++;                   // remove leading quote

    char* unescaped = str_unescape(content);
    value* result = value_new_string(unescaped);
    free(unescaped);

    return result;
}

static int value_parse_symbol(char* input, value** v, size_t offset) {
    char* running = input;
    while (*running != '\0' && strchr(symbol_chars, *running)) {
        running++;
    }

    size_t length = running - input;
    char* symbol = malloc(length + 1);
    strncpy(symbol, input, length);
    symbol[length] = '\0';

    if (is_number(symbol)) {
        *v = value_read_number(symbol, offset);
    } else {
        *v = value_new_symbol(symbol);
    }

    free(symbol);

    return length;
}

static int value_parse_special(char* input, value** v, size_t offset) {
    char* running = input + 1;
    while (*running != '\0' && strchr(alpha_chars, *running)) {
        running++;
    }

    size_t length = running - input;
    char* special = malloc(length + 1);
    strncpy(special, input, length);
    special[length] = '\0';

    *v = value_read_special(special, offset);

    free(special);

    return length;
}

static int value_parse_string(char* input, value** v, size_t offset) {
    char* running = input + 1;
    while (!(*running == '\"' && *(running - 1) != '\\')) {
        if (*running == '\0') {
            *v = create_parsing_error(offset, "unterminated string");
            return running - input;
        }
        running++;
    }

    running++;  // skip the trailing quote
    size_t length = running - input;
    char* string = malloc(length + 1);
    strncpy(string, input, length);
    string[length] = '\0';

    *v = value_read_string(string, offset);

    free(string);

    return length;
}

static int value_parse_expr(char* input, value* v, char end, size_t offset) {
    size_t pos = 0;
    char* running = input;
    while (*running != end) {
        pos = offset + (running - input);
        if (*running == '\0') {
            value* error = create_parsing_error(pos, "missing '%c'", end);
            value_add_child(v, error);
            break;
        } else if (strchr(whitespace_chars, *running)) {
            running++;
        } else if (strchr("})", *running)) {
            value* error = create_parsing_error(pos, "premature '%c'", *running);
            value_add_child(v, error);
            break;
        } else if (*running == ';') {
            // comment till the end of the line
            while (!strchr("\r\n\0", *running)) {
                running++;
            }
        } else if (*running == '(') {
            running++;
            value* sexpr = value_new_sexpr();
            running += value_parse_expr(running, sexpr, ')', pos + 1) + 1;
            value_add_child(v, sexpr);
        } else if (*running == '{') {
            running++;
            value* qexpr = value_new_qexpr();
            running += value_parse_expr(running, qexpr, '}', pos + 1) + 1;
            value_add_child(v, qexpr);
        } else if (*running == '#') {
            value* special = NULL;
            running += value_parse_special(running, &special, pos);
            value_add_child(v, special);
        } else if (*running == '\"') {
            value* string = NULL;
            running += value_parse_string(running, &string, pos);
            value_add_child(v, string);
        } else if (strchr(symbol_chars, *running)) {
            value* symbol = NULL;
            running += value_parse_symbol(running, &symbol, pos);
            value_add_child(v, symbol);
        } else {
            value* error = create_parsing_error(pos, "unexpected symbol '%c'", *running);
            value_add_child(v, error);
            running += strlen(running);
            break;
        }
    }

    return running - input;
}

static value* find_error(value* v) {
    if (v->type == VALUE_ERROR) {
        return v;
    } else if (v->type == VALUE_SEXPR || v->type == VALUE_QEXPR) {
        for (size_t i = 0; i < v->num_children; i++) {
            value* e = find_error(v->children[i]);
            if (e != NULL && e->type == VALUE_ERROR) {
                return e;
            }
        }
        return NULL;
    } else {
        return NULL;
    }
}

value* value_parse(char* input) {
    value* v = value_new_sexpr();
    value_parse_expr(input, v, 0, 1);

    value* e = find_error(v);
    if (e != NULL) {
        value* temp = value_new_error(e->symbol);
        value_dispose(v);
        v = temp;
    }

    return v;
}
