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

static void test_number_output(parser* p, char* input, double expected) {
    value* e = get_evaluated(p, input);

    if (e != NULL) {
        assert(e->type == VALUE_NUMBER);
        assert(e->number == expected);
        value_dispose(e);
    }
}

static void test_error_output(parser* p, char* input, char* expected) {
    value* e = get_evaluated(p, input);

    if (e != NULL) {
        assert(e->type == VALUE_ERROR);
        assert(strstr(e->error, expected));
        value_dispose(e);
    }
}

static void test_str_output(parser* p, char* input, char* expected) {
    value* e = get_evaluated(p, input);

    if (e != NULL) {
        char buffer[1024];
        value_to_str(e, buffer);
        assert(strcmp(buffer, expected) == 0);
        value_dispose(e);
    }
}

static void test_numeric(parser* p) {
    test_number_output(p, "+ 1", 1);
    test_number_output(p, "+ -1", -1);
    test_number_output(p, "+ 0", 0);
    test_number_output(p, "+ 1 2 3", 6);
    test_number_output(p, "- 1", -1);
    test_number_output(p, "- -1", 1);
    test_number_output(p, "- 1 2 3", -4);
    test_number_output(p, "+ 1 (- 2 3) 4", 4);
    test_number_output(p, " +   1 (-  2    3)    4 ", 4);
    test_number_output(p, "* 3.14 -2.71", -8.5094);
    test_number_output(p, "* 1 2 3 4 5", 120);
    test_number_output(p, "/ 1 2", 0.5);
    test_number_output(p, "/ -3 4", -0.75);
    test_number_output(p, "% 11 3", 2);
    test_number_output(p, "% 11.5 3.2", 2);
    test_number_output(p, "^ 2 10", 1024);
    test_number_output(p, "^ 2 -10", 1. / 1024);
    test_number_output(p, "+ 1 (* 2 3) 4 (/ 10 5) (- 6 (% 8 7)) 9", 27);
    test_number_output(p, "+ 0 1 2 3 4 5 6 7 8 9 10", 55);
    test_number_output(p, "* 0 1 2 3 4 5 6 7 8 9 10", 0);
    test_number_output(p, "* 1 2 3 4 5 6 7 8 9 10", 3628800);
    test_number_output(p, "* -1 2 -3 4 -5 6 -7 8 -9 10", -3628800);
    test_number_output(p, "min 1 3 -5", -5);
    test_number_output(p, "max 10 0 -1", 10);
    test_number_output(p, "min (max 1 3 5) (max 2 4 6)", 5);
    test_number_output(p, "5", 5);
    test_number_output(p, "(5)", 5);
    test_number_output(p, "(+ 1 2 3 (- 4 5) 6)", 11);
    printf("\n");
}

static void test_errors(parser* p) {
    test_error_output(p, "/ 1 0", "division by zero");
    test_error_output(p, "+ 1 (/ 2 0) 3", "division by zero");
    test_error_output(p, "fake 1 2 3", "unrecognizer operator");
    test_error_output(p, "1 2 3", "does not start with a symbol");
    test_error_output(p, "1 2 3", "(1 2 3)");
    test_error_output(p, "(1 2 3)", "does not start with a symbol");
    test_error_output(p, "(1 2 3)", "(1 2 3)");
    test_error_output(p, "+ 1 2 3 -", "arg #3 (-) must be of type number");
    test_error_output(p, "+ 1 2 3 {4 5}", "arg #3 ({4 5}) must be of type number");
    printf("\n");
}

static void test_str(parser* p) {
    test_str_output(p, "", "()");
    test_str_output(p, "  ", "()");
    test_str_output(p, "+", "+");
    test_str_output(p, "min", "min");
    test_str_output(p, "fake", "fake");
    test_str_output(p, "-5", "-5");
    test_str_output(p, "(-3.14)", "-3.14");
    test_str_output(p, "{}", "{}");
    test_str_output(p, "{1}", "{1}");
    test_str_output(p, "{1 2 3}", "{1 2 3}");
    test_str_output(p, "{+ 1 2 3}", "{+ 1 2 3}");
    test_str_output(p, "{1 2 3 +}", "{1 2 3 +}");
    test_str_output(p, "{+ 1 2 3 {- 4 5} 6}", "{+ 1 2 3 {- 4 5} 6}");
    test_str_output(p, "{+ 1 2 3 (- 4 5) 6}", "{+ 1 2 3 (- 4 5) 6}");
    printf("\n");
}

static void test_list(parser* p) {
    test_str_output(p, "list 1 2 3", "{1 2 3}");
    test_str_output(p, "list {1 2 3}", "{{1 2 3}}");
    test_str_output(p, "list + - * /", "{+ - * /}");
    test_str_output(p, "list 0", "{0}");
    test_str_output(p, "list", "list");
    test_str_output(p, "list list", "{list}");
    test_str_output(p, "(list 1 2 3)", "{1 2 3}");
    test_str_output(p, "{list 1 2 3}", "{list 1 2 3}");
    printf("\n");
}

static void test_head(parser* p) {
    test_str_output(p, "head {1 2 3}", "{1}");
    test_str_output(p, "head {1}", "{1}");
    test_str_output(p, "head {+}", "{+}");
    test_str_output(p, "head {+ + + -}", "{+}");
    test_str_output(p, "head {head + + + -}", "{head}");
    test_error_output(p, "head 1", "arg #0 (1) must be of type q-expr");
    test_error_output(p, "head {}", "arg #0 ({}) must be at least 1-long");
    test_error_output(p, "head 1 2 3", "expects exactly 1 arg");
    printf("\n");
}

static void test_tail(parser* p) {
    test_str_output(p, "tail {1}", "{}");
    test_str_output(p, "tail {1 2 3}", "{2 3}");
    test_str_output(p, "tail {+}", "{}");
    test_str_output(p, "tail {+ 1}", "{1}");
    test_str_output(p, "tail {1 + 2 -}", "{+ 2 -}");
    test_str_output(p, "tail {tail tail tail}", "{tail tail}");
    test_error_output(p, "tail 2", "arg #0 (2) must be of type q-expr");
    test_error_output(p, "tail {}", "arg #0 ({}) must be at least 1-long");
    test_error_output(p, "tail {1} {2} {3}", "expects exactly 1 arg");
    printf("\n");
}

static void test_join(parser* p) {
    test_str_output(p, "join {}", "{}");
    test_str_output(p, "join {} {}", "{}");
    test_str_output(p, "join {} {} {}", "{}");
    test_str_output(p, "join {1} {2}", "{1 2}");
    test_str_output(p, "join {1} {2 3} {(4 5) /}", "{1 2 3 (4 5) /}");
    test_str_output(p, "join {1} {2 3} {(4 5) /} {}", "{1 2 3 (4 5) /}");
    test_error_output(p, "join {1} {2 3} 5 {(4 5) /} {}", "arg #2 (5) must be of type q-expr");
    test_error_output(p, "join 1 2 3", "arg #0 (1) must be of type q-expr");
    printf("\n");
}

static void test_eval(parser* p) {
    test_number_output(p, "eval {+ 1 2 3}", 6);
    test_str_output(p, "eval {}", "()");
    test_str_output(p, "eval {+}", "+");
    test_str_output(p, "eval {list {1 2 3}}", "{{1 2 3}}");
    test_str_output(p, "eval {list 1 2 3} ", "{1 2 3}");
    test_str_output(p, "eval {eval {list + 2 3}}", "{+ 2 3}");
    test_str_output(p, "eval {head (list 1 2 3 4)}", "{1}");
    test_str_output(p, "eval (tail {tail tail {5 6 7}})", "{6 7}");
    test_number_output(p, "eval (head {(+ 1 2) (+ 10 20)})", 3);
    test_number_output(p, "eval (eval {list + 2 3})", 5);
    test_error_output(p, "eval {1} {2}", "expects exactly 1 arg");
    test_error_output(p, "eval 3.14", "arg #0 (3.14) must be of type q-expr");
    printf("\n");
}

static void test_cons(parser* p) {
    test_str_output(p, "cons 1 {}", "{1}");
    test_str_output(p, "cons 1 {2 3}", "{1 2 3}");
    test_str_output(p, "cons {1} {2 3}", "{{1} 2 3}");
    test_str_output(p, "cons + {1 2 3}", "{+ 1 2 3}");
    test_number_output(p, "eval (cons + {1 2 3})", 6);
    test_str_output(p, "cons", "cons");
    test_str_output(p, "cons {} {}", "{{}}");
    test_error_output(p, "cons 1", "expects exactly 2 args");
    test_error_output(p, "cons {}", "expects exactly 2 args");
    test_error_output(p, "cons 1 2 3", "expects exactly 2 args");
    test_error_output(p, "cons 1 2", "arg #1 (2) must be of type q-expr");
    test_error_output(p, "cons {} 2", "arg #1 (2) must be of type q-expr");
    printf("\n");
}

static void test_len(parser* p) {
    test_number_output(p, "len {}", 0);
    test_number_output(p, "len {1}", 1);
    test_number_output(p, "len {1 2 3}", 3);
    test_number_output(p, "len {{1} {2 3 4 5}}", 2);
    test_error_output(p, "len 1", "arg #0 (1) must be of type q-expr");
    test_error_output(p, "len +", "arg #0 (+) must be of type q-expr");
    test_error_output(p, "len {} {}", "expects exactly 1 arg");
    printf("\n");
}

static void test_init(parser* p) {
    test_str_output(p, "init {1}", "{}");
    test_str_output(p, "init {1 2 3}", "{1 2}");
    test_str_output(p, "init {{1} {2 3} {4}}", "{{1} {2 3}}");
    test_str_output(p, "init {{1} (+ 2 3) {4}}", "{{1} (+ 2 3)}");
    test_str_output(p, "init {+ - * /}", "{+ - *}");
    test_error_output(p, "init {}", "arg #0 ({}) must be at least 1-long");
    test_error_output(p, "init 1", "arg #0 (1) must be of type q-expr");
    test_error_output(p, "init {1} {2}", "expects exactly 1 arg");
    printf("\n");
}

void run_test(parser* p) {
    counter = 0;

    test_numeric(p);
    test_errors(p);
    test_str(p);

    test_list(p);
    test_head(p);
    test_tail(p);
    test_join(p);
    test_eval(p);
    test_cons(p);
    test_len(p);
    test_init(p);
}
