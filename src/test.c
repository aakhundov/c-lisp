#include "test.h"

#include <assert.h>

#include "env.h"
#include "eval.h"
#include "parse.h"
#include "value.h"

static int counter = 0;

static value* get_evaluated(parser* p, environment* env, char* input) {
    result r;
    char output[128];

    if (parser_parse(p, input, &r)) {
        tree t = result_get_tree(&r);
        value* v = value_from_tree(&t);
        value* e = value_evaluate(v, env);

        result_dispose_tree(&r);
        value_dispose(v);

        value_to_str(e, output);
        printf("%-5d \"%s\" --> \"%s\"\n", ++counter, input, output);

        return e;
    } else {
        result_print_error(&r);
        result_dispose_error(&r);

        exit(1);
    }
}

static void test_number_output(parser* p, environment* env, char* input, double expected) {
    value* e = get_evaluated(p, env, input);

    if (e != NULL) {
        assert(e->type == VALUE_NUMBER);
        assert(e->number == expected);
        value_dispose(e);
    }
}

static void test_error_output(parser* p, environment* env, char* input, char* expected) {
    value* e = get_evaluated(p, env, input);

    if (e != NULL) {
        assert(e->type == VALUE_ERROR);
        assert(strstr(e->error, expected));
        value_dispose(e);
    }
}

static void test_str_output(parser* p, environment* env, char* input, char* expected) {
    value* e = get_evaluated(p, env, input);

    if (e != NULL) {
        char buffer[1024];
        value_to_str(e, buffer);
        assert(strcmp(buffer, expected) == 0);
        value_dispose(e);
    }
}

static void test_numeric(parser* p, environment* env) {
    test_number_output(p, env, "+ 1", 1);
    test_number_output(p, env, "+ -1", -1);
    test_number_output(p, env, "+ 0", 0);
    test_number_output(p, env, "+ 1 2 3", 6);
    test_number_output(p, env, "- 1", -1);
    test_number_output(p, env, "- -1", 1);
    test_number_output(p, env, "- 1 2 3", -4);
    test_number_output(p, env, "+ 1 (- 2 3) 4", 4);
    test_number_output(p, env, " +   1 (-  2    3)    4 ", 4);
    test_number_output(p, env, "* 3.14 -2.71", -8.5094);
    test_number_output(p, env, "* 1 2 3 4 5", 120);
    test_number_output(p, env, "/ 1 2", 0.5);
    test_number_output(p, env, "/ -3 4", -0.75);
    test_number_output(p, env, "% 11 3", 2);
    test_number_output(p, env, "% 11.5 3.2", 2);
    test_number_output(p, env, "^ 2 10", 1024);
    test_number_output(p, env, "^ 2 -10", 1. / 1024);
    test_number_output(p, env, "+ 1 (* 2 3) 4 (/ 10 5) (- 6 (% 8 7)) 9", 27);
    test_number_output(p, env, "+ 0 1 2 3 4 5 6 7 8 9 10", 55);
    test_number_output(p, env, "* 0 1 2 3 4 5 6 7 8 9 10", 0);
    test_number_output(p, env, "* 1 2 3 4 5 6 7 8 9 10", 3628800);
    test_number_output(p, env, "* -1 2 -3 4 -5 6 -7 8 -9 10", -3628800);
    test_number_output(p, env, "min 1 3 -5", -5);
    test_number_output(p, env, "max 10 0 -1", 10);
    test_number_output(p, env, "min (max 1 3 5) (max 2 4 6)", 5);
    test_number_output(p, env, "5", 5);
    test_number_output(p, env, "(5)", 5);
    test_number_output(p, env, "(+ 1 2 3 (- 4 5) 6)", 11);
    printf("\n");
}

static void test_errors(parser* p, environment* env) {
    test_error_output(p, env, "/ 1 0", "division by zero");
    test_error_output(p, env, "+ 1 (/ 2 0) 3", "division by zero");
    test_error_output(p, env, "fake 1 2 3", "undefined symbol: fake");
    test_error_output(p, env, "1 2 3", "must start with a function");
    test_error_output(p, env, "1 2 3", "(1 2 3)");
    test_error_output(p, env, "(1 2 3)", "must start with a function");
    test_error_output(p, env, "(1 2 3)", "(1 2 3)");
    test_error_output(p, env, "+ 1 2 3 -", "arg #3 (<function ->) must be of type number");
    test_error_output(p, env, "+ 1 2 3 {4 5}", "arg #3 ({4 5}) must be of type number");
    printf("\n");
}

static void test_str(parser* p, environment* env) {
    test_str_output(p, env, "", "()");
    test_str_output(p, env, "  ", "()");
    test_str_output(p, env, "+", "<function +>");
    test_str_output(p, env, "min", "<function min>");
    test_str_output(p, env, "fake", "error: undefined symbol: fake");
    test_str_output(p, env, "-5", "-5");
    test_str_output(p, env, "(-3.14)", "-3.14");
    test_str_output(p, env, "{}", "{}");
    test_str_output(p, env, "{1}", "{1}");
    test_str_output(p, env, "{1 2 3}", "{1 2 3}");
    test_str_output(p, env, "{+ 1 2 3}", "{+ 1 2 3}");
    test_str_output(p, env, "{1 2 3 +}", "{1 2 3 +}");
    test_str_output(p, env, "{+ 1 2 3 {- 4 5} 6}", "{+ 1 2 3 {- 4 5} 6}");
    test_str_output(p, env, "{+ 1 2 3 (- 4 5) 6}", "{+ 1 2 3 (- 4 5) 6}");
    printf("\n");
}

static void test_list(parser* p, environment* env) {
    test_str_output(p, env, "list 1 2 3", "{1 2 3}");
    test_str_output(p, env, "list {1 2 3}", "{{1 2 3}}");
    test_str_output(p, env, "list + - * /", "{<function +> <function -> <function *> <function />}");
    test_str_output(p, env, "list 0", "{0}");
    test_str_output(p, env, "list", "<function list>");
    test_str_output(p, env, "list list", "{<function list>}");
    test_str_output(p, env, "(list 1 2 3)", "{1 2 3}");
    test_str_output(p, env, "{list 1 2 3}", "{list 1 2 3}");
    printf("\n");
}

static void test_head(parser* p, environment* env) {
    test_str_output(p, env, "head {1 2 3}", "{1}");
    test_str_output(p, env, "head {1}", "{1}");
    test_str_output(p, env, "head {+}", "{+}");
    test_str_output(p, env, "head {+ + + -}", "{+}");
    test_str_output(p, env, "head {head + + + -}", "{head}");

    test_error_output(p, env, "head 1", "arg #0 (1) must be of type q-expr");
    test_error_output(p, env, "head {}", "arg #0 ({}) must be at least 1-long");
    test_error_output(p, env, "head 1 2 3", "expects exactly 1 arg");
    printf("\n");
}

static void test_tail(parser* p, environment* env) {
    test_str_output(p, env, "tail {1}", "{}");
    test_str_output(p, env, "tail {1 2 3}", "{2 3}");
    test_str_output(p, env, "tail {+}", "{}");
    test_str_output(p, env, "tail {+ 1}", "{1}");
    test_str_output(p, env, "tail {1 + 2 -}", "{+ 2 -}");
    test_str_output(p, env, "tail {tail tail tail}", "{tail tail}");

    test_error_output(p, env, "tail 2", "arg #0 (2) must be of type q-expr");
    test_error_output(p, env, "tail {}", "arg #0 ({}) must be at least 1-long");
    test_error_output(p, env, "tail {1} {2} {3}", "expects exactly 1 arg");
    printf("\n");
}

static void test_join(parser* p, environment* env) {
    test_str_output(p, env, "join {}", "{}");
    test_str_output(p, env, "join {} {}", "{}");
    test_str_output(p, env, "join {} {} {}", "{}");
    test_str_output(p, env, "join {1} {2}", "{1 2}");
    test_str_output(p, env, "join {1} {2 3} {(4 5) /}", "{1 2 3 (4 5) /}");
    test_str_output(p, env, "join {1} {2 3} {(4 5) /} {}", "{1 2 3 (4 5) /}");

    test_error_output(p, env, "join {1} {2 3} 5 {(4 5) /} {}", "arg #2 (5) must be of type q-expr");
    test_error_output(p, env, "join 1 2 3", "arg #0 (1) must be of type q-expr");
    printf("\n");
}

static void test_eval(parser* p, environment* env) {
    test_number_output(p, env, "eval {+ 1 2 3}", 6);
    test_str_output(p, env, "eval {}", "()");
    test_str_output(p, env, "eval {+}", "<function +>");
    test_str_output(p, env, "eval {list {1 2 3}}", "{{1 2 3}}");
    test_str_output(p, env, "eval {list 1 2 3} ", "{1 2 3}");
    test_str_output(p, env, "eval {eval {list + 2 3}}", "{<function +> 2 3}");
    test_str_output(p, env, "eval {head (list 1 2 3 4)}", "{1}");
    test_str_output(p, env, "eval (tail {tail tail {5 6 7}})", "{6 7}");
    test_number_output(p, env, "eval (head {(+ 1 2) (+ 10 20)})", 3);
    test_number_output(p, env, "eval (eval {list + 2 3})", 5);

    test_error_output(p, env, "eval {1} {2}", "expects exactly 1 arg");
    test_error_output(p, env, "eval 3.14", "arg #0 (3.14) must be of type q-expr");
    printf("\n");
}

static void test_cons(parser* p, environment* env) {
    test_str_output(p, env, "cons 1 {}", "{1}");
    test_str_output(p, env, "cons 1 {2 3}", "{1 2 3}");
    test_str_output(p, env, "cons {1} {2 3}", "{{1} 2 3}");
    test_str_output(p, env, "cons + {1 2 3}", "{<function +> 1 2 3}");
    test_number_output(p, env, "eval (cons + {1 2 3})", 6);
    test_str_output(p, env, "cons", "<function cons>");
    test_str_output(p, env, "cons {} {}", "{{}}");

    test_error_output(p, env, "cons 1", "expects exactly 2 args");
    test_error_output(p, env, "cons {}", "expects exactly 2 args");
    test_error_output(p, env, "cons 1 2 3", "expects exactly 2 args");
    test_error_output(p, env, "cons 1 2", "arg #1 (2) must be of type q-expr");
    test_error_output(p, env, "cons {} 2", "arg #1 (2) must be of type q-expr");
    printf("\n");
}

static void test_len(parser* p, environment* env) {
    test_number_output(p, env, "len {}", 0);
    test_number_output(p, env, "len {1}", 1);
    test_number_output(p, env, "len {1 2 3}", 3);
    test_number_output(p, env, "len {{1} {2 3 4 5}}", 2);

    test_error_output(p, env, "len 1", "arg #0 (1) must be of type q-expr");
    test_error_output(p, env, "len +", "arg #0 (<function +>) must be of type q-expr");
    test_error_output(p, env, "len {} {}", "expects exactly 1 arg");
    printf("\n");
}

static void test_init(parser* p, environment* env) {
    test_str_output(p, env, "init {1}", "{}");
    test_str_output(p, env, "init {1 2 3}", "{1 2}");
    test_str_output(p, env, "init {{1} {2 3} {4}}", "{{1} {2 3}}");
    test_str_output(p, env, "init {{1} (+ 2 3) {4}}", "{{1} (+ 2 3)}");
    test_str_output(p, env, "init {+ - * /}", "{+ - *}");

    test_error_output(p, env, "init {}", "arg #0 ({}) must be at least 1-long");
    test_error_output(p, env, "init 1", "arg #0 (1) must be of type q-expr");
    test_error_output(p, env, "init {1} {2}", "expects exactly 1 arg");
    printf("\n");
}

static void test_def(parser* p, environment* env) {
    test_error_output(p, env, "two", "undefined symbol");
    test_str_output(p, env, "def {two} 2", "()");
    test_str_output(p, env, "two", "2");
    test_error_output(p, env, "pi", "undefined symbol");
    test_error_output(p, env, "times", "undefined symbol");
    test_error_output(p, env, "some", "undefined symbol");
    test_str_output(p, env, "def {pi times some} 3.14 * {xyz}", "()");
    test_str_output(p, env, "pi", "3.14");
    test_str_output(p, env, "times", "<function *>");
    test_str_output(p, env, "some", "{xyz}");
    test_number_output(p, env, "times two pi", 6.28);
    test_error_output(p, env, "arglist", "undefined symbol");
    test_str_output(p, env, "def {arglist} {a b x y}", "()");
    test_str_output(p, env, "arglist", "{a b x y}");
    test_str_output(p, env, "def arglist 1 2 3 4", "()");
    test_str_output(p, env, "list a b x y", "{1 2 3 4}");
    test_number_output(p, env, "eval (join {+} (list a b x y))", 10);

    test_error_output(p, env, "def {a}", "expects at least 2 args");
    test_error_output(p, env, "def 1 2", "arg #0 (1) must be of type q-expr");
    test_error_output(p, env, "def {} 1", "arg #0 ({}) must be at least 1-long");
    test_error_output(p, env, "def {a b} 1", "expects exactly 3 args");
    test_error_output(p, env, "def {a b c} 1", "expects exactly 4 args");
    test_error_output(p, env, "def {a b c} 1 2", "expects exactly 4 args");
    test_error_output(p, env, "def {1} 2", "first argument must consist of symbols");
    test_error_output(p, env, "def {a 1} 2 3", "first argument must consist of symbols");
    printf("\n");
}

void run_test(parser* p, environment* env) {
    counter = 0;

    test_numeric(p, env);
    test_errors(p, env);
    test_str(p, env);
    test_list(p, env);
    test_head(p, env);
    test_tail(p, env);
    test_join(p, env);
    test_eval(p, env);
    test_cons(p, env);
    test_len(p, env);
    test_init(p, env);
    test_def(p, env);
}
