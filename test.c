#include "test.h"

#include <assert.h>

#include "eval.h"
#include "parse.h"
#include "value.h"

static value* get_evaluated(parser* p, char* input) {
    result r;
    char output[128];

    if (parser_parse(p, input, &r)) {
        tree t = result_get_tree(&r);
        value* v = value_from_tree(&t);
        value* e = value_evaluate(v);

        result_dispose_tree(&r);
        value_dispose(v);

        value_to_str(e, output);
        printf("\"%s\" --> \"%s\"\n", input, output);

        return e;
    } else {
        result_print_error(&r);
        result_dispose_error(&r);

        return NULL;
    }
}

static void test_number(parser* p, char* input, double expected) {
    value* e = get_evaluated(p, input);

    if (e != NULL) {
        assert(e->type == VALUE_NUMBER);
        assert(e->number == expected);
        value_dispose(e);
    }
}

static void test_error(parser* p, char* input, char* expected) {
    value* e = get_evaluated(p, input);

    if (e != NULL) {
        assert(e->type == VALUE_ERROR);
        assert(strstr(e->error, expected));
        value_dispose(e);
    }
}

static void test_str(parser* p, char* input, char* expected) {
    value* e = get_evaluated(p, input);

    if (e != NULL) {
        char buffer[1024];
        value_to_str(e, buffer);
        assert(strcmp(buffer, expected) == 0);
        value_dispose(e);
    }
}

void run_test(parser* p) {
    test_number(p, "+ 1", 1);
    test_number(p, "+ -1", -1);
    test_number(p, "+ 0", 0);
    test_number(p, "+ 1 2 3", 6);
    test_number(p, "- 1", -1);
    test_number(p, "- -1", 1);
    test_number(p, "- 1 2 3", -4);
    test_number(p, "+ 1 (- 2 3) 4", 4);
    test_number(p, " +   1 (-  2    3)    4 ", 4);
    test_number(p, "* 3.14 -2.71", -8.5094);
    test_number(p, "* 1 2 3 4 5", 120);
    test_number(p, "/ 1 2", 0.5);
    test_number(p, "/ -3 4", -0.75);
    test_number(p, "% 11 3", 2);
    test_number(p, "% 11.5 3.2", 2);
    test_number(p, "^ 2 10", 1024);
    test_number(p, "^ 2 -10", 1. / 1024);
    test_number(p, "+ 1 (* 2 3) 4 (/ 10 5) (- 6 (% 8 7)) 9", 27);
    test_number(p, "+ 0 1 2 3 4 5 6 7 8 9 10", 55);
    test_number(p, "* 0 1 2 3 4 5 6 7 8 9 10", 0);
    test_number(p, "* 1 2 3 4 5 6 7 8 9 10", 3628800);
    test_number(p, "* -1 2 -3 4 -5 6 -7 8 -9 10", -3628800);
    test_number(p, "min 1 3 -5", -5);
    test_number(p, "max 10 0 -1", 10);
    test_number(p, "min (max 1 3 5) (max 2 4 6)", 5);
    test_number(p, "5", 5);
    test_number(p, "(5)", 5);
    test_number(p, "(+ 1 2 3 (- 4 5) 6)", 11);
    printf("\n");

    test_error(p, "/ 1 0", "division by zero");
    test_error(p, "+ 1 (/ 2 0) 3", "division by zero");
    test_error(p, "fake 1 2 3", "unrecognizer operator");
    test_error(p, "1 2 3", "s-expr doesn't start with");
    test_error(p, "1 2 3", "(1 2 3)");
    test_error(p, "(1 2 3)", "s-expr doesn't start with");
    test_error(p, "(1 2 3)", "(1 2 3)");
    test_error(p, "+ 1 2 3 -", "non-numeric argument: '-'");
    test_error(p, "+ 1 2 3 {4 5}", "non-numeric argument: '{4 5}'");
    printf("\n");

    test_str(p, "", "()");
    test_str(p, "  ", "()");
    test_str(p, "+", "+");
    test_str(p, "min", "min");
    test_str(p, "fake", "fake");
    test_str(p, "-5", "-5");
    test_str(p, "(-3.14)", "-3.14");
    test_str(p, "{}", "{}");
    test_str(p, "{1}", "{1}");
    test_str(p, "{1 2 3}", "{1 2 3}");
    test_str(p, "{+ 1 2 3}", "{+ 1 2 3}");
    test_str(p, "{1 2 3 +}", "{1 2 3 +}");
    test_str(p, "{+ 1 2 3 {- 4 5} 6}", "{+ 1 2 3 {- 4 5} 6}");
    test_str(p, "{+ 1 2 3 (- 4 5) 6}", "{+ 1 2 3 (- 4 5) 6}");
}
