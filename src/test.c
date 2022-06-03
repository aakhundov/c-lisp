#include "test.h"

#include <assert.h>

#include "eval.h"
#include "parse.h"
#include "value.h"

static int counter = 0;

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
        printf("%-5d \"%s\" --> \"%s\"\n", ++counter, input, output);

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
    counter = 0;

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
    test_error(p, "1 2 3", "does not start with a symbol");
    test_error(p, "1 2 3", "(1 2 3)");
    test_error(p, "(1 2 3)", "does not start with a symbol");
    test_error(p, "(1 2 3)", "(1 2 3)");
    test_error(p, "+ 1 2 3 -", "arg #3 (-) must be of type number");
    test_error(p, "+ 1 2 3 {4 5}", "arg #3 ({4 5}) must be of type number");
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
    printf("\n");

    test_str(p, "list 1 2 3", "{1 2 3}");
    test_str(p, "list {1 2 3}", "{{1 2 3}}");
    test_str(p, "list + - * /", "{+ - * /}");
    test_str(p, "list 0", "{0}");
    test_str(p, "list", "list");
    test_str(p, "list list", "{list}");
    test_str(p, "(list 1 2 3)", "{1 2 3}");
    test_str(p, "{list 1 2 3}", "{list 1 2 3}");
    printf("\n");

    test_str(p, "head {1 2 3}", "{1}");
    test_str(p, "head {1}", "{1}");
    test_str(p, "head {+}", "{+}");
    test_str(p, "head {+ + + -}", "{+}");
    test_str(p, "head {head + + + -}", "{head}");
    test_error(p, "head 1", "arg #0 (1) must be of type q-expr");
    test_error(p, "head {}", "arg #0 ({}) must be at least 1-long");
    test_error(p, "head 1 2 3", "expects exactly 1 arg");
    printf("\n");

    test_str(p, "tail {1}", "{}");
    test_str(p, "tail {1 2 3}", "{2 3}");
    test_str(p, "tail {+}", "{}");
    test_str(p, "tail {+ 1}", "{1}");
    test_str(p, "tail {1 + 2 -}", "{+ 2 -}");
    test_str(p, "tail {tail tail tail}", "{tail tail}");
    test_error(p, "tail 2", "arg #0 (2) must be of type q-expr");
    test_error(p, "tail {}", "arg #0 ({}) must be at least 1-long");
    test_error(p, "tail {1} {2} {3}", "expects exactly 1 arg");
    printf("\n");

    test_str(p, "join {}", "{}");
    test_str(p, "join {} {}", "{}");
    test_str(p, "join {} {} {}", "{}");
    test_str(p, "join {1} {2}", "{1 2}");
    test_str(p, "join {1} {2 3} {(4 5) /}", "{1 2 3 (4 5) /}");
    test_str(p, "join {1} {2 3} {(4 5) /} {}", "{1 2 3 (4 5) /}");
    test_error(p, "join {1} {2 3} 5 {(4 5) /} {}", "arg #2 (5) must be of type q-expr");
    test_error(p, "join 1 2 3", "arg #0 (1) must be of type q-expr");
    printf("\n");

    test_number(p, "eval {+ 1 2 3}", 6);
    test_str(p, "eval {}", "()");
    test_str(p, "eval {+}", "+");
    test_str(p, "eval {list {1 2 3}}", "{{1 2 3}}");
    test_str(p, "eval {list 1 2 3} ", "{1 2 3}");
    test_str(p, "eval {eval {list + 2 3}}", "{+ 2 3}");
    test_str(p, "eval {head (list 1 2 3 4)}", "{1}");
    test_str(p, "eval (tail {tail tail {5 6 7}})", "{6 7}");
    test_number(p, "eval (head {(+ 1 2) (+ 10 20)})", 3);
    test_number(p, "eval (eval {list + 2 3})", 5);
    test_error(p, "eval {1} {2}", "expects exactly 1 arg");
    test_error(p, "eval 3.14", "arg #0 (3.14) must be of type q-expr");
    printf("\n");

    test_str(p, "cons 1 {}", "{1}");
    test_str(p, "cons 1 {2 3}", "{1 2 3}");
    test_str(p, "cons {1} {2 3}", "{{1} 2 3}");
    test_str(p, "cons + {1 2 3}", "{+ 1 2 3}");
    test_number(p, "eval (cons + {1 2 3})", 6);
    test_str(p, "cons", "cons");
    test_str(p, "cons {} {}", "{{}}");
    test_error(p, "cons 1", "expects exactly 2 args");
    test_error(p, "cons {}", "expects exactly 2 args");
    test_error(p, "cons 1 2 3", "expects exactly 2 args");
    test_error(p, "cons 1 2", "arg #1 (2) must be of type q-expr");
    test_error(p, "cons {} 2", "arg #1 (2) must be of type q-expr");
    printf("\n");

    test_number(p, "len {}", 0);
    test_number(p, "len {1}", 1);
    test_number(p, "len {1 2 3}", 3);
    test_number(p, "len {{1} {2 3 4 5}}", 2);
    test_error(p, "len 1", "arg #0 (1) must be of type q-expr");
    test_error(p, "len +", "arg #0 (+) must be of type q-expr");
    test_error(p, "len {} {}", "expects exactly 1 arg");
    printf("\n");

    test_str(p, "init {1}", "{}");
    test_str(p, "init {1 2 3}", "{1 2}");
    test_str(p, "init {{1} {2 3} {4}}", "{{1} {2 3}}");
    test_str(p, "init {{1} (+ 2 3) {4}}", "{{1} (+ 2 3)}");
    test_str(p, "init {+ - * /}", "{+ - *}");
    test_error(p, "init {}", "arg #0 ({}) must be at least 1-long");
    test_error(p, "init 1", "arg #0 (1) must be of type q-expr");
    test_error(p, "init {1} {2}", "expects exactly 1 arg");
    printf("\n");
}
