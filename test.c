#include "test.h"

#include <assert.h>

#include "eval.h"
#include "parse.h"
#include "value.h"

static value get_value(parser* p, char* input) {
    result r;
    parser_parse(p, input, &r);
    tree t = result_get_tree(&r);
    value v = evaluate(&t);

    char output[128];
    value_to_str(&v, output);
    printf("%s  ==>  %s\n", input, output);

    return v;
}

static void test_number(parser* p, char* input, double expected) {
    value v = get_value(p, input);
    assert(v.type == VALUE_NUMBER);
    assert(v.number == expected);
}

static void test_error(parser* p, char* input, error_type expected) {
    value v = get_value(p, input);
    assert(v.type == VALUE_ERROR);
    assert(v.error == expected);
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
    test_number(p, "* 3.14 -2.71", -8.5094);
    test_number(p, "* 1 2 3 4 5", 120);
    test_number(p, "/ 1 2", 0.5);
    test_number(p, "/ -3 4", -0.75);
    test_number(p, "% 11 3", 2);
    test_number(p, "% 11.5 3.2", 2);
    test_number(p, "^ 2 10", 1024);
    test_number(p, "^ 2 -10", 1. / 1024);
    test_number(p, "+ 1 (* 2 3) 4 (/ 10 5) (- 6 (% 8 7)) 9", 27);
    test_number(p, "min 1 3 -5", -5);
    test_number(p, "max 10 0 -1", 10);
    test_number(p, "min (max 1 3 5) (max 2 4 6)", 5);

    test_error(p, "/ 1 0", ERROR_DIV_ZERO);
    test_error(p, "+ 1 (/ 2 0) 3", ERROR_DIV_ZERO);
    test_error(p, "fake 1 2 3", ERROR_BAD_OP);
}
