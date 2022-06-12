#include "test.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "env.h"
#include "eval.h"
#include "value.h"

#define RUN_TEST_FN(fn)                        \
    {                                          \
        printf("[%s]\n", #fn);                 \
        printf("=========================\n"); \
        environment env;                       \
        environment_init(&env);                \
        environment_register_builtins(&env);   \
        fn(&env);                              \
        environment_dispose(&env);             \
        printf("\n");                          \
    }

static int counter = 0;

static value* get_evaluated(environment* env, char* input) {
    char output[1024];

    int parsed = 1;
    value* v = value_parse(input);
    if (v->type != VALUE_ERROR) {
        value* e = value_evaluate(v, env);
        value_dispose(v);
        v = e;
    } else {
        parsed = 0;
    }

    value_to_str(v, output);
    printf(
        "\x1B[34m%-5d\x1B[0m "
        "\x1B[34m[\x1B[0m%s\x1B[34m]\x1B[0m "
        "\x1B[34m-->\x1B[0m "
        "\x1B[34m[\x1B[0m%s\x1B[34m]\x1B[0m\n",
        ++counter, input, output);

    if (parsed) {
        return v;
    } else {
        value_dispose(v);
        exit(1);
    }
}

static void test_number_output(environment* env, char* input, double expected) {
    value* e = get_evaluated(env, input);

    if (e != NULL) {
        assert(e->type == VALUE_NUMBER);
        assert(e->number == expected);
        value_dispose(e);
    }
}

static void test_error_output(environment* env, char* input, char* expected) {
    value* e = get_evaluated(env, input);

    if (e != NULL) {
        assert(e->type == VALUE_ERROR);
        assert(strstr(e->symbol, expected));
        value_dispose(e);
    }
}

static void test_info_output(environment* env, char* input, char* expected) {
    value* e = get_evaluated(env, input);

    if (e != NULL) {
        assert(e->type == VALUE_INFO);
        assert(strstr(e->symbol, expected));
        value_dispose(e);
    }
}

static void test_bool_output(environment* env, char* input, int expected) {
    value* e = get_evaluated(env, input);

    if (e != NULL) {
        assert(e->type == VALUE_BOOL);
        assert(e->number == expected);
        value_dispose(e);
    }
}

static void test_full_output(environment* env, char* input, char* expected) {
    value* e = get_evaluated(env, input);

    if (e != NULL) {
        char buffer[1024];
        value_to_str(e, buffer);
        assert(strcmp(buffer, expected) == 0);
        value_dispose(e);
    }
}

static void test_numeric(environment* env) {
    test_number_output(env, "+ 1", 1);
    test_number_output(env, "+ -1", -1);
    test_number_output(env, "+ 0", 0);
    test_number_output(env, "+ 1 2 3", 6);
    test_number_output(env, "- 1", -1);
    test_number_output(env, "- -1", 1);
    test_number_output(env, "- 1 2 3", -4);
    test_number_output(env, "+ 1 (- 2 3) 4", 4);
    test_number_output(env, " +   1 (-  2    3)    4 ", 4);
    test_number_output(env, "* 3.14 -2.71", -8.5094);
    test_number_output(env, "* 1 2 3 4 5", 120);
    test_number_output(env, "/ 1 2", 0.5);
    test_number_output(env, "/ -3 4", -0.75);
    test_number_output(env, "% 11 3", 2);
    test_number_output(env, "% 11.5 3.2", 2);
    test_number_output(env, "^ 2 10", 1024);
    test_number_output(env, "^ 2 -10", 1. / 1024);
    test_number_output(env, "+ 1 (* 2 3) 4 (/ 10 5) (- 6 (% 8 7)) 9", 27);
    test_number_output(env, "+ 0 1 2 3 4 5 6 7 8 9 10", 55);
    test_number_output(env, "* 0 1 2 3 4 5 6 7 8 9 10", 0);
    test_number_output(env, "* 1 2 3 4 5 6 7 8 9 10", 3628800);
    test_number_output(env, "* -1 2 -3 4 -5 6 -7 8 -9 10", -3628800);
    test_number_output(env, "min 1 3 -5", -5);
    test_number_output(env, "max 10 0 -1", 10);
    test_number_output(env, "min (max 1 3 5) (max 2 4 6)", 5);
    test_number_output(env, "5", 5);
    test_number_output(env, "(5)", 5);
    test_number_output(env, "(+ 1 2 3 (- 4 5) 6)", 11);
}

static void test_errors(environment* env) {
    test_error_output(env, "/ 1 0", "division by zero");
    test_error_output(env, "+ 1 (/ 2 0) 3", "division by zero");
    test_error_output(env, "fake 1 2 3", "undefined symbol: fake");
    test_error_output(env, "1 2 3", "must start with a function");
    test_error_output(env, "1 2 3", "(1 2 3)");
    test_error_output(env, "(1 2 3)", "must start with a function");
    test_error_output(env, "(1 2 3)", "(1 2 3)");
    test_error_output(env, "+ 1 2 3 -", "arg #3 (<builtin ->) must be of type number");
    test_error_output(env, "+ 1 2 3 {4 5}", "arg #3 ({4 5}) must be of type number");
}

static void test_full(environment* env) {
    test_full_output(env, "", "()");
    test_full_output(env, "  ", "()");
    test_full_output(env, "+", "<builtin +>");
    test_full_output(env, "min", "<builtin min>");
    test_full_output(env, "-5", "-5");
    test_full_output(env, "(-3.14)", "-3.14");
    test_full_output(env, "{}", "{}");
    test_full_output(env, "{1}", "{1}");
    test_full_output(env, "{1 2 3}", "{1 2 3}");
    test_full_output(env, "{+ 1 2 3}", "{+ 1 2 3}");
    test_full_output(env, "{1 2 3 +}", "{1 2 3 +}");
    test_full_output(env, "{+ 1 2 3 {- 4 5} 6}", "{+ 1 2 3 {- 4 5} 6}");
    test_full_output(env, "{+ 1 2 3 (- 4 5) 6}", "{+ 1 2 3 (- 4 5) 6}");
}

static void test_special(environment* env) {
    test_bool_output(env, "#true", 1);
    test_bool_output(env, "#false", 0);
    test_full_output(env, "#true", "#true");
    test_full_output(env, "#false", "#false");
    test_full_output(env, "#null", "{}");
}

static void test_string(environment* env) {
    test_full_output(env, "\"\"", "\"\"");
    test_full_output(env, "\"a\"", "\"a\"");
    test_full_output(env, "\" \"", "\" \"");
    test_full_output(env, "\"   \"", "\"   \"");
    test_full_output(env, "\"abc\"", "\"abc\"");
    test_full_output(env, "\"abc def\"", "\"abc def\"");
    test_full_output(env, "\"abc\\\"def\"", "\"abc\\\"def\"");
    test_full_output(env, "\"\\\"\"", "\"\\\"\"");
    test_full_output(env, "\" \\\"\\\"  \"", "\" \\\"\\\"  \"");
    test_full_output(env, "\"'abc'\"", "\"'abc'\"");
    test_full_output(env, "\"abc\\n\"", "\"abc\\n\"");
    test_full_output(env, "\"\\r\\n\"", "\"\\r\\n\"");
    test_full_output(env, "\"abc\\ndef\"", "\"abc\\ndef\"");
    test_full_output(env, "\"abc\\0def\"", "\"abc\"");
}

static void test_comment(environment* env) {
    test_full_output(env, ";", "()");
    test_full_output(env, "; comment", "()");
    test_number_output(env, "1 ; comment", 1);
    test_full_output(env, "+; comment", "<builtin +>");
    test_number_output(env, "+ 1 2 3 ; comment", 6);
}

static void test_list(environment* env) {
    test_full_output(env, "list 1 2 3", "{1 2 3}");
    test_full_output(env, "list {1 2 3}", "{{1 2 3}}");
    test_full_output(env, "list + - * /", "{<builtin +> <builtin -> <builtin *> <builtin />}");
    test_full_output(env, "list 0", "{0}");
    test_full_output(env, "list", "<builtin list>");
    test_full_output(env, "list list", "{<builtin list>}");
    test_full_output(env, "(list 1 2 3)", "{1 2 3}");
    test_full_output(env, "{list 1 2 3}", "{list 1 2 3}");
}

static void test_first(environment* env) {
    test_number_output(env, "first {1 2 3}", 1);
    test_full_output(env, "first {1}", "1");
    test_full_output(env, "first {+ 1 2 3}", "+");
    test_full_output(env, "first {{+ 1} {2 3}}", "{+ 1}");
    test_full_output(env, "first {(+ 1) {2 3}}", "(+ 1)");

    test_error_output(env, "first 1", "arg #0 (1) must be of type q-expr");
    test_error_output(env, "first {}", "arg #0 ({}) must be at least 1-long");
    test_error_output(env, "first 1 2 3", "expects exactly 1 arg");
}

static void test_head(environment* env) {
    test_full_output(env, "head {1 2 3}", "{1}");
    test_full_output(env, "head {1}", "{1}");
    test_full_output(env, "head {+}", "{+}");
    test_full_output(env, "head {+ + + -}", "{+}");
    test_full_output(env, "head {head + + + -}", "{head}");

    test_error_output(env, "head 1", "arg #0 (1) must be of type q-expr");
    test_error_output(env, "head {}", "arg #0 ({}) must be at least 1-long");
    test_error_output(env, "head 1 2 3", "expects exactly 1 arg");
}

static void test_tail(environment* env) {
    test_full_output(env, "tail {1}", "{}");
    test_full_output(env, "tail {1 2 3}", "{2 3}");
    test_full_output(env, "tail {+}", "{}");
    test_full_output(env, "tail {+ 1}", "{1}");
    test_full_output(env, "tail {1 + 2 -}", "{+ 2 -}");
    test_full_output(env, "tail {tail tail tail}", "{tail tail}");

    test_error_output(env, "tail 2", "arg #0 (2) must be of type q-expr");
    test_error_output(env, "tail {}", "arg #0 ({}) must be at least 1-long");
    test_error_output(env, "tail {1} {2} {3}", "expects exactly 1 arg");
}

static void test_join(environment* env) {
    test_full_output(env, "join {}", "{}");
    test_full_output(env, "join {} {}", "{}");
    test_full_output(env, "join {} {} {}", "{}");
    test_full_output(env, "join {1} {2}", "{1 2}");
    test_full_output(env, "join {1} {2 3} {(4 5) /}", "{1 2 3 (4 5) /}");
    test_full_output(env, "join {1} {2 3} {(4 5) /} {}", "{1 2 3 (4 5) /}");

    test_error_output(env, "join {1} {2 3} 5 {(4 5) /} {}", "arg #2 (5) must be of type q-expr");
    test_error_output(env, "join 1 2 3", "arg #0 (1) must be of type q-expr");
}

static void test_eval(environment* env) {
    test_number_output(env, "eval {+ 1 2 3}", 6);
    test_full_output(env, "eval {}", "()");
    test_full_output(env, "eval {+}", "<builtin +>");
    test_full_output(env, "eval {list {1 2 3}}", "{{1 2 3}}");
    test_full_output(env, "eval {list 1 2 3} ", "{1 2 3}");
    test_full_output(env, "eval {eval {list + 2 3}}", "{<builtin +> 2 3}");
    test_full_output(env, "eval {head (list 1 2 3 4)}", "{1}");
    test_full_output(env, "eval (tail {tail tail {5 6 7}})", "{6 7}");
    test_number_output(env, "eval (head {(+ 1 2) (+ 10 20)})", 3);
    test_number_output(env, "eval (eval {list + 2 3})", 5);

    test_error_output(env, "eval {1} {2}", "expects exactly 1 arg");
    test_error_output(env, "eval 3.14", "arg #0 (3.14) must be of type q-expr");
}

static void test_cons(environment* env) {
    test_full_output(env, "cons 1 {}", "{1}");
    test_full_output(env, "cons 1 {2 3}", "{1 2 3}");
    test_full_output(env, "cons {1} {2 3}", "{{1} 2 3}");
    test_full_output(env, "cons + {1 2 3}", "{<builtin +> 1 2 3}");
    test_number_output(env, "eval (cons + {1 2 3})", 6);
    test_full_output(env, "cons", "<builtin cons>");
    test_full_output(env, "cons {} {}", "{{}}");

    test_error_output(env, "cons 1", "expects exactly 2 args");
    test_error_output(env, "cons {}", "expects exactly 2 args");
    test_error_output(env, "cons 1 2 3", "expects exactly 2 args");
    test_error_output(env, "cons 1 2", "arg #1 (2) must be of type q-expr");
    test_error_output(env, "cons {} 2", "arg #1 (2) must be of type q-expr");
}

static void test_len(environment* env) {
    test_number_output(env, "len {}", 0);
    test_number_output(env, "len {1}", 1);
    test_number_output(env, "len {1 2 3}", 3);
    test_number_output(env, "len {{1} {2 3 4 5}}", 2);

    test_error_output(env, "len 1", "arg #0 (1) must be of type q-expr");
    test_error_output(env, "len +", "arg #0 (<builtin +>) must be of type q-expr");
    test_error_output(env, "len {} {}", "expects exactly 1 arg");
}

static void test_init(environment* env) {
    test_full_output(env, "init {1}", "{}");
    test_full_output(env, "init {1 2 3}", "{1 2}");
    test_full_output(env, "init {{1} {2 3} {4}}", "{{1} {2 3}}");
    test_full_output(env, "init {{1} (+ 2 3) {4}}", "{{1} (+ 2 3)}");
    test_full_output(env, "init {+ - * /}", "{+ - *}");

    test_error_output(env, "init {}", "arg #0 ({}) must be at least 1-long");
    test_error_output(env, "init 1", "arg #0 (1) must be of type q-expr");
    test_error_output(env, "init {1} {2}", "expects exactly 1 arg");
}

static void test_def(environment* env) {
    test_error_output(env, "two", "undefined symbol");
    test_info_output(env, "def {two} 2", "defined: two");
    test_full_output(env, "two", "2");
    test_error_output(env, "pi", "undefined symbol");
    test_error_output(env, "times", "undefined symbol");
    test_error_output(env, "some", "undefined symbol");
    test_info_output(env, "def {pi times some} 3.14 * {xyz}", "defined: pi times some");
    test_full_output(env, "pi", "3.14");
    test_full_output(env, "times", "<builtin times>");
    test_full_output(env, "some", "{xyz}");
    test_number_output(env, "times two pi", 6.28);
    test_error_output(env, "arglist", "undefined symbol");
    test_info_output(env, "def {arglist} {one two three four}", "defined: arglist");
    test_full_output(env, "arglist", "{one two three four}");
    test_info_output(env, "def arglist 1 2 3 4", "defined: one two three four");
    test_full_output(env, "list one two three four", "{1 2 3 4}");
    test_number_output(env, "eval (join {+} (list one two three four))", 10);

    test_error_output(env, "def {a}", "expects at least 2 args");
    test_error_output(env, "def 1 2", "arg #0 (1) must be of type q-expr");
    test_error_output(env, "def {} 1", "arg #0 ({}) must be at least 1-long");
    test_error_output(env, "def {a b} 1", "expects exactly 3 args");
    test_error_output(env, "def {a b c} 1", "expects exactly 4 args");
    test_error_output(env, "def {a b c} 1 2", "expects exactly 4 args");
    test_error_output(env, "def {1} 2", "arg #0 ({1}) must consist of symbol children");
    test_error_output(env, "def {a 1} 2 3", "arg #0 ({a 1}) must consist of symbol children");
}

static void test_lambda(environment* env) {
    test_full_output(env, "lambda", "<builtin lambda>");
    test_full_output(env, "lambda {x} {x}", "<lambda {x} {x}>");
    test_full_output(env, "lambda {} {x}", "<lambda {} {x}>");
    test_full_output(env, "lambda {x y} {+ x y}", "<lambda {x y} {+ x y}>");

    test_error_output(env, "lambda 1", "expects exactly 2 args");
    test_error_output(env, "lambda {x}", "expects exactly 2 args");
    test_error_output(env, "lambda {x} {x} {x}", "expects exactly 2 args");
    test_error_output(env, "lambda 1 2", "arg #0 (1) must be of type q-expr");
    test_error_output(env, "lambda {x} 2", "arg #1 (2) must be of type q-expr");
    test_error_output(env, "lambda 1 {x}", "arg #0 (1) must be of type q-expr");
    test_error_output(env, "lambda {1} {x}", "arg #0 ({1}) must consist of symbol children");
    test_error_output(env, "lambda {x &} {1}", "exactly one argument must follow &");
    test_error_output(env, "lambda {x & y z} {1}", "exactly one argument must follow &");
}

static void test_parent_env(environment* env) {
    environment cenv;
    environment_init(&cenv);
    cenv.parent = env;

    test_error_output(env, "global-var", "undefined symbol: global-var");
    test_error_output(&cenv, "global-var", "undefined symbol: global-var");
    test_info_output(&cenv, "def {global-var} 1", "defined: global-var");
    test_number_output(env, "global-var", 1);
    test_number_output(&cenv, "global-var", 1);

    test_error_output(env, "local-var", "undefined symbol: local-var");
    test_error_output(&cenv, "local-var", "undefined symbol: local-var");
    test_info_output(&cenv, "local {local-var} 1", "defined: local-var");
    test_error_output(env, "local-var", "undefined symbol: local-var");
    test_number_output(&cenv, "local-var", 1);

    environment_dispose(&cenv);
}

static void test_function_call(environment* env) {
    test_info_output(env, "def {fn-negate} (lambda {x} {- x})", "defined: fn-negate");
    test_info_output(env, "def {fn-restore} (lambda {x} {- (fn-negate x)})", "defined: fn-restore");
    test_info_output(env, "def {fn-add} (lambda {x y z} {+ x y z})", "defined: fn-add");
    test_info_output(env, "def {fn-add-mul} (lambda {x y} {+ x (* x y)})", "defined: fn-add-mul");
    test_info_output(env, "def {fn-pack} (lambda {x & y} {join (list x) y})", "defined: fn-pack");
    test_info_output(env, "def {fn-curry} (lambda {f args} {eval (join (list f) args)})", "defined: fn-curry");
    test_info_output(env, "def {fn-uncurry} (lambda {f & args} {f args})", "defined: fn-uncurry");
    test_info_output(env, "def {fn-wrong} (lambda {x} {+ x y})", "defined: fn-wrong");

    test_number_output(env, "fn-negate 1", -1);
    test_number_output(env, "fn-negate -3.14", 3.14);
    test_number_output(env, "fn-restore -3.14", -3.14);
    test_number_output(env, "fn-add 1 2 3", 6);
    test_number_output(env, "fn-add 0 0 0", 0);
    test_number_output(env, "fn-add-mul 10 20", 210);
    test_number_output(env, "fn-add-mul -7 5", -42);
    test_full_output(env, "fn-pack 1", "{1}");
    test_full_output(env, "fn-pack 1 2 3", "{1 2 3}");
    test_number_output(env, "fn-curry + {1 2 3}", 6);
    test_number_output(env, "fn-curry * {10 20}", 200);
    test_full_output(env, "fn-uncurry head 1 2 3", "{1}");
    test_number_output(env, "fn-uncurry len 1 2 3", 3);
    test_full_output(env, "fn-uncurry tail 1", "{}");

    test_error_output(env, "fn-negate 1 2", "expects exactly 1 arg");
    test_error_output(env, "fn-add 1 2", "expects exactly 3 args");
    test_error_output(env, "fn-add 1 2 3 4", "expects exactly 3 args");
    test_error_output(env, "fn-wrong 1", "undefined symbol: y");
}

static void test_fn(environment* env) {
    test_info_output(env, "fn {fx-negate x} {- x}", "defined: fx-negate");
    test_info_output(env, "fn {fx-restore x} {- (fx-negate x)}", "defined: fx-restore");
    test_info_output(env, "fn {fx-add x y z} {+ x y z}", "defined: fx-add");
    test_info_output(env, "fn {fx-add-mul x y} {+ x (* x y)}", "defined: fx-add-mul");
    test_info_output(env, "fn {fx-pack x & y} {join (list x) y}", "defined: fx-pack");
    test_info_output(env, "fn {fx-curry f args} {eval (join (list f) args)}", "defined: fx-curry");
    test_info_output(env, "fn {fx-uncurry f & args} {f args}", "defined: fx-uncurry");
    test_info_output(env, "fn {fx-wrong x} {+ x y}", "defined: fx-wrong");

    test_number_output(env, "fx-negate 1", -1);
    test_number_output(env, "fx-negate -3.14", 3.14);
    test_number_output(env, "fx-restore -3.14", -3.14);
    test_number_output(env, "fx-add 1 2 3", 6);
    test_number_output(env, "fx-add 0 0 0", 0);
    test_number_output(env, "fx-add-mul 10 20", 210);
    test_number_output(env, "fx-add-mul -7 5", -42);
    test_full_output(env, "fx-pack 1", "{1}");
    test_full_output(env, "fx-pack 1 2 3", "{1 2 3}");
    test_number_output(env, "fx-curry + {1 2 3}", 6);
    test_number_output(env, "fx-curry * {10 20}", 200);
    test_full_output(env, "fx-uncurry head 1 2 3", "{1}");
    test_number_output(env, "fx-uncurry len 1 2 3", 3);
    test_full_output(env, "fx-uncurry tail 1", "{}");

    test_error_output(env, "fx-negate 1 2", "expects exactly 1 arg");
    test_error_output(env, "fx-add 1 2", "expects exactly 3 args");
    test_error_output(env, "fx-add 1 2 3 4", "expects exactly 3 args");
    test_error_output(env, "fx-wrong 1", "undefined symbol: y");

    test_error_output(env, "fn 1", "expects exactly 2 args");
    test_error_output(env, "fn {f x}", "expects exactly 2 args");
    test_error_output(env, "fn {f x} {x} {x}", "expects exactly 2 args");
    test_error_output(env, "fn 1 2", "arg #0 (1) must be of type q-expr");
    test_error_output(env, "fn {f x} 2", "arg #1 (2) must be of type q-expr");
    test_error_output(env, "fn 1 {x}", "arg #0 (1) must be of type q-expr");
    test_error_output(env, "fn {} {1}", "arg #0 ({}) must be at least 1-long");
    test_error_output(env, "fn {f 1} {2}", "arg #0 ({f 1}) must consist of symbol children");
    test_error_output(env, "fn {f x &} {1}", "exactly one argument must follow &");
    test_error_output(env, "fn {f x & y z} {1}", "exactly one argument must follow &");
}

static void test_del(environment* env) {
    test_full_output(env, "+", "<builtin +>");
    test_info_output(env, "del {+}", "deleted: +");
    test_error_output(env, "+", "undefined symbol: +");

    test_error_output(env, "xyz", "undefined symbol: xyz");
    test_info_output(env, "def {xyz} 123", "defined: xyz");
    test_number_output(env, "xyz", 123);
    test_info_output(env, "del {xyz}", "deleted: xyz");
    test_error_output(env, "xyz", "undefined symbol: xyz");

    test_error_output(env, "del {x} {y}", "expects exactly 1 arg");
    test_error_output(env, "del 1", "arg #0 (1) must be of type q-expr");
    test_error_output(env, "del {}", "arg #0 ({}) must be exactly 1-long");
    test_error_output(env, "del {x y}", "arg #0 ({x y}) must be exactly 1-long");
    test_error_output(env, "del {1}", "arg #0 ({1}) must consist of symbol children");
    test_error_output(env, "del {abc}", "not found: abc");
}

static void test_eq(environment* env) {
    test_bool_output(env, "== 1 1", 1);
    test_bool_output(env, "== 1 2", 0);
    test_bool_output(env, "== 1 1 1 1 1", 1);
    test_bool_output(env, "== 1 1 1 1 2", 0);
    test_bool_output(env, "== 2 1 1 1 1", 0);
    test_bool_output(env, "== 1 2 3 4 5", 0);
    test_bool_output(env, "== {a} {a}", 1);
    test_bool_output(env, "== {a} {b}", 0);
    test_bool_output(env, "== {a} {a a}", 0);
    test_bool_output(env, "== {a {b}} {a {b}}", 1);
    test_bool_output(env, "== {a {c}} {a {b}}", 0);
    test_bool_output(env, "== {a {#true {1 2 3}}} {a {#true {1 2 3}}}", 1);
    test_bool_output(env, "== + add", 1);
    test_bool_output(env, "== + add -", 0);
    test_bool_output(env, "== + -", 0);
    test_bool_output(env, "== (lambda {x y} {+ x y}) (lambda {x y} {+ x y})", 1);
    test_bool_output(env, "== (lambda {x y} {+ x y}) (lambda {x z} {+ x z})", 0);
    test_bool_output(env, "== (lambda {x y} {+ x y}) (lambda {x y} {- x y})", 0);
    test_bool_output(env, "== #true #false", 0);
    test_bool_output(env, "== #true #true", 1);

    test_error_output(env, "== 1", "expects at least 2 args");
    test_error_output(env, "== {a}", "expects at least 2 args");
}

static void test_neq(environment* env) {
    test_bool_output(env, "!= 1 1", 0);
    test_bool_output(env, "!= 1 2", 1);
    test_bool_output(env, "!= 1 1 1 1 1", 0);
    test_bool_output(env, "!= 1 1 1 1 2", 0);
    test_bool_output(env, "!= 2 1 1 1 1", 0);
    test_bool_output(env, "!= 1 2 3 4 1", 0);
    test_bool_output(env, "!= 1 2 3 4 5", 1);
    test_bool_output(env, "!= {a} {a}", 0);
    test_bool_output(env, "!= {a} {b}", 1);
    test_bool_output(env, "!= {a} {a a}", 1);
    test_bool_output(env, "!= {a} {a} {a a}", 0);
    test_bool_output(env, "!= {a {b}} {a {b}}", 0);
    test_bool_output(env, "!= {a {c}} {a {b}} {a {c}}", 0);
    test_bool_output(env, "!= {a {c}} {a {b}}", 1);
    test_bool_output(env, "!= {a {#true {1 2 3}}} {a {#true {1 2 3}}}", 0);
    test_bool_output(env, "!= + - * /", 1);
    test_bool_output(env, "!= + - * / add", 0);
    test_bool_output(env, "!= (lambda {x y} {+ x y}) (lambda {x y} {+ x y})", 0);
    test_bool_output(env, "!= (lambda {x y} {+ x y}) (lambda {x z} {+ x z})", 1);
    test_bool_output(env, "!= (lambda {x y} {+ x y}) (lambda {x y} {- x y})", 1);
    test_bool_output(env, "!= #true #false", 1);
    test_bool_output(env, "!= #true #true", 0);

    test_error_output(env, "!= 1", "expects at least 2 args");
    test_error_output(env, "!= {a}", "expects at least 2 args");
}

static void test_gt(environment* env) {
    test_bool_output(env, "> 1 2", 0);
    test_bool_output(env, "> 1 1", 0);
    test_bool_output(env, "> 2 1", 1);
    test_bool_output(env, "> 5 4 3 2 1", 1);
    test_bool_output(env, "> 5 4 3 2 2", 0);
    test_bool_output(env, "> 5 5 3 2 1", 0);
    test_bool_output(env, "> 1 2 3 4 5", 0);
    test_bool_output(env, "> 5 3 4 2 1", 0);
    test_bool_output(env, "> {} {}", 0);
    test_bool_output(env, "> {a} {}", 1);
    test_bool_output(env, "> {} {a}", 0);
    test_bool_output(env, "> {a} {b}", 0);
    test_bool_output(env, "> {a} {a}", 0);
    test_bool_output(env, "> {b} {a}", 1);
    test_bool_output(env, "> {a a} {a}", 1);
    test_bool_output(env, "> {a} {a a}", 0);
    test_bool_output(env, "> {abc} {abb}", 1);
    test_bool_output(env, "> {abc} {abc}", 0);
    test_bool_output(env, "> {abc} {abd}", 0);

    test_error_output(env, "> 1", "expects at least 2 args");
    test_error_output(env, "> {a}", "expects at least 2 args");
    test_error_output(env, "> 1 {a}", "can't compare values of different types");
    test_error_output(env, "> #true #false", "incomprable type");
    test_error_output(env, "> + -", "incomprable type");
}

static void test_gte(environment* env) {
    test_bool_output(env, ">= 1 2", 0);
    test_bool_output(env, ">= 1 1", 1);
    test_bool_output(env, ">= 2 1", 1);
    test_bool_output(env, ">= 5 4 3 2 1", 1);
    test_bool_output(env, ">= 5 4 3 2 2", 1);
    test_bool_output(env, ">= 5 5 3 2 1", 1);
    test_bool_output(env, ">= 1 2 3 4 5", 0);
    test_bool_output(env, ">= 5 3 4 2 1", 0);
    test_bool_output(env, ">= {} {}", 1);
    test_bool_output(env, ">= {a} {}", 1);
    test_bool_output(env, ">= {} {a}", 0);
    test_bool_output(env, ">= {a} {b}", 0);
    test_bool_output(env, ">= {a} {a}", 1);
    test_bool_output(env, ">= {b} {a}", 1);
    test_bool_output(env, ">= {a a} {a}", 1);
    test_bool_output(env, ">= {a} {a a}", 0);
    test_bool_output(env, ">= {abc} {abb}", 1);
    test_bool_output(env, ">= {abc} {abc}", 1);
    test_bool_output(env, ">= {abc} {abd}", 0);

    test_error_output(env, ">= 1", "expects at least 2 args");
    test_error_output(env, ">= {a}", "expects at least 2 args");
    test_error_output(env, ">= 1 {a}", "can't compare values of different types");
    test_error_output(env, ">= #true #false", "incomprable type");
    test_error_output(env, ">= + -", "incomprable type");
}

static void test_lt(environment* env) {
    test_bool_output(env, "< 1 2", 1);
    test_bool_output(env, "< 1 1", 0);
    test_bool_output(env, "< 2 1", 0);
    test_bool_output(env, "< 1 2 3 4 5", 1);
    test_bool_output(env, "< 2 2 3 4 5", 0);
    test_bool_output(env, "< 1 2 3 5 5", 0);
    test_bool_output(env, "< 5 4 3 2 1", 0);
    test_bool_output(env, "< 1 2 3 5 4", 0);
    test_bool_output(env, "< {} {}", 0);
    test_bool_output(env, "< {} {a}", 1);
    test_bool_output(env, "< {a} {}", 0);
    test_bool_output(env, "< {a} {b}", 1);
    test_bool_output(env, "< {a} {a}", 0);
    test_bool_output(env, "< {b} {a}", 0);
    test_bool_output(env, "< {a} {a a}", 1);
    test_bool_output(env, "< {a a} {a}", 0);
    test_bool_output(env, "< {abc} {abb}", 0);
    test_bool_output(env, "< {abc} {abc}", 0);
    test_bool_output(env, "< {abc} {abd}", 1);

    test_error_output(env, "< 1", "expects at least 2 args");
    test_error_output(env, "< {a}", "expects at least 2 args");
    test_error_output(env, "< 1 {a}", "can't compare values of different types");
    test_error_output(env, "< #true #false", "incomprable type");
    test_error_output(env, "< + -", "incomprable type");
}

static void test_lte(environment* env) {
    test_bool_output(env, "<= 1 2", 1);
    test_bool_output(env, "<= 1 1", 1);
    test_bool_output(env, "<= 2 1", 0);
    test_bool_output(env, "<= 1 2 3 4 5", 1);
    test_bool_output(env, "<= 2 2 3 4 5", 1);
    test_bool_output(env, "<= 1 2 3 5 5", 1);
    test_bool_output(env, "<= 5 4 3 2 1", 0);
    test_bool_output(env, "<= 1 2 3 5 4", 0);
    test_bool_output(env, "<= {} {}", 1);
    test_bool_output(env, "<= {} {a}", 1);
    test_bool_output(env, "<= {a} {}", 0);
    test_bool_output(env, "<= {a} {b}", 1);
    test_bool_output(env, "<= {a} {a}", 1);
    test_bool_output(env, "<= {b} {a}", 0);
    test_bool_output(env, "<= {a} {a a}", 1);
    test_bool_output(env, "<= {a a} {a}", 0);
    test_bool_output(env, "<= {abc} {abb}", 0);
    test_bool_output(env, "<= {abc} {abc}", 1);
    test_bool_output(env, "<= {abc} {abd}", 1);

    test_error_output(env, "<= 1", "expects at least 2 args");
    test_error_output(env, "<= {a}", "expects at least 2 args");
    test_error_output(env, "<= 1 {a}", "can't compare values of different types");
    test_error_output(env, "<= #true #false", "incomprable type");
    test_error_output(env, "<= + -", "incomprable type");
}

static void test_null_q(environment* env) {
    test_bool_output(env, "null? {}", 1);
    test_bool_output(env, "null? {1}", 0);
    test_bool_output(env, "null? {1 2 3}", 0);
    test_bool_output(env, "null? {a}", 0);
    test_bool_output(env, "null? {+ -}", 0);
    test_bool_output(env, "null? 0", 1);
    test_bool_output(env, "null? 1", 0);
    test_bool_output(env, "null? -1", 0);
    test_bool_output(env, "null? \"\"", 1);
    test_bool_output(env, "null? \"a\"", 0);

    test_error_output(env, "null? {} {1}", "expects exactly 1 arg");
    test_error_output(env, "null? {} {1} 2", "expects exactly 1 arg");
}

static void test_zero_q(environment* env) {
    test_bool_output(env, "zero? 0", 1);
    test_bool_output(env, "zero? 0.0", 1);
    test_bool_output(env, "zero? 1", 0);
    test_bool_output(env, "zero? -1", 0);
    test_bool_output(env, "zero? 3.14", 0);
    test_bool_output(env, "zero? .222", 0);

    test_error_output(env, "zero? {} {1}", "expects exactly 1 arg");
    test_error_output(env, "zero? {} {1} 2", "expects exactly 1 arg");
    test_error_output(env, "zero? {}", "arg #0 ({}) must be of type number");
}

static void test_list_q(environment* env) {
    test_bool_output(env, "list? {}", 1);
    test_bool_output(env, "list? {1}", 1);
    test_bool_output(env, "list? {1 2 3}", 1);
    test_bool_output(env, "list? {a}", 1);
    test_bool_output(env, "list? {+ -}", 1);
    test_bool_output(env, "list? 1", 0);
    test_bool_output(env, "list? 3.14", 0);
    test_bool_output(env, "list? +", 0);
    test_bool_output(env, "list? list?", 0);

    test_error_output(env, "list? {} {1}", "expects exactly 1 arg");
    test_error_output(env, "list? {} {1} 2", "expects exactly 1 arg");
}

static void test_if(environment* env) {
    test_number_output(env, "if #true {1} {0}", 1);
    test_number_output(env, "if #false {1} {0}", 0);
    test_number_output(env, "if (< 10 20) {1} {0}", 1);
    test_number_output(env, "if (> 10 20) {1} {0}", 0);
    test_number_output(env, "if 1 {1} {0}", 1);
    test_number_output(env, "if -1 {1} {0}", 1);
    test_number_output(env, "if 0 {1} {0}", 0);
    test_number_output(env, "if {a} {1} {0}", 1);
    test_number_output(env, "if {} {1} {0}", 0);
    test_number_output(env, "if #true {/ 1 1} {/ 1 0}", 1);
    test_error_output(env, "if #false {/ 1 1} {/ 1 0}", "division by zero");
    test_error_output(env, "if + {1} {0}", "can't cast function to bool");
    test_error_output(env, "if (fn {f x} {- x}) {1} {0}", "can't cast info to bool");

    test_info_output(env, "fn {positive? x} {if (> x 0) {#true} {#false}}", "defined: positive?");
    test_bool_output(env, "positive? -100", 0);
    test_bool_output(env, "positive? -3.14", 0);
    test_bool_output(env, "positive? -1", 0);
    test_bool_output(env, "positive? 0", 0);
    test_bool_output(env, "positive? 1", 1);
    test_bool_output(env, "positive? 3.14", 1);
    test_bool_output(env, "positive? 100", 1);

    test_error_output(env, "if #true", "expects exactly 3 args");
    test_error_output(env, "if #true {1}", "expects exactly 3 args");
    test_error_output(env, "if #true {1} {0} {-1}", "expects exactly 3 args");
    test_error_output(env, "if #true 1 {0}", "arg #1 (1) must be of type q-expr");
    test_error_output(env, "if #true {1} 0", "arg #2 (0) must be of type q-expr");
}

static void test_cond(environment* env) {
    test_number_output(env, "cond #true {1}", 1);
    test_full_output(env, "cond #false {0}", "{}");
    test_number_output(env, "cond (< 10 20) {1} (> 10 20) {0}", 1);
    test_number_output(env, "cond (> 10 20) {1} (< 10 20) {0}", 0);
    test_full_output(env, "cond (> 10 20) {1} (> 5 20) {0}", "{}");
    test_number_output(env, "cond 1 {/ 1 1} 0 {/ 1 0}", 1);
    test_error_output(env, "cond 0 {/ 1 1} 1 {/ 1 0}", "division by zero");
    test_number_output(env, "cond 1 {1} (/ 1 0) {0}", 1);
    test_error_output(env, "cond 0 {1} (/ 1 0) {0}", "division by zero");

    test_info_output(env, "fn {sign x} {cond (< x 0) {-1} (== x 0) {0} #true {1}}", "defined: sign");
    test_number_output(env, "sign -100", -1);
    test_number_output(env, "sign -3.14", -1);
    test_number_output(env, "sign -1", -1);
    test_number_output(env, "sign 0", 0);
    test_number_output(env, "sign 1", 1);
    test_number_output(env, "sign 3.14", 1);
    test_number_output(env, "sign 100", 1);

    test_error_output(env, "cond #true", "expects at least 2 args");
    test_error_output(env, "cond #true {1} #false", "expects an even number of args");
    test_error_output(env, "cond #true 1 #false {0}", "arg #1 (1) must be of type q-expr");
    test_error_output(env, "cond #true {1} #false 0", "arg #3 (0) must be of type q-expr");
}

static void test_recursion(environment* env) {
    test_info_output(env,
                     "\n"
                     "fn {fact n} {\n"
                     "  if (<= n 1)\n"
                     "    {1}\n"
                     "    {* n (fact (- n 1))}\n"
                     "}\n",
                     "defined: fact");
    test_number_output(env, "fact 1", 1);
    test_number_output(env, "fact 3", 6);
    test_number_output(env, "fact 5", 120);
    test_number_output(env, "fact 10", 3628800);
    test_number_output(env, "fact 0", 1);
    test_number_output(env, "fact -10", 1);

    test_info_output(env,
                     "\n"
                     "fn {length lst} {\n"
                     "  if (null? lst)\n"
                     "    {0}\n"
                     "    {+ 1 (length (tail lst))}\n"
                     "}\n",
                     "defined: length");
    test_number_output(env, "length {}", 0);
    test_number_output(env, "length {a}", 1);
    test_number_output(env, "length {1 2 3}", 3);
    test_number_output(env, "length {{1 2} 3 {4 5 6}}", 3);

    test_info_output(env,
                     "\n"
                     "fn {reverse lst} {\n"
                     "  if (null? lst)\n"
                     "    {{}}\n"
                     "    {join (reverse (tail lst)) (head lst)}\n"
                     "}\n",
                     "defined: reverse");
    test_full_output(env, "reverse {}", "{}");
    test_full_output(env, "reverse {a}", "{a}");
    test_full_output(env, "reverse {+ - * /}", "{/ * - +}");
    test_full_output(env, "reverse {{1 2} 3 {4 5 6}}", "{{4 5 6} 3 {1 2}}");

    test_info_output(env,
                     "\n"
                     "fn {flatten lst} {\n"
                     "  cond\n"
                     "    (null? lst) {{}}\n"
                     "    (list? (car lst)) {join (flatten (car lst)) (flatten (cdr lst))}\n"
                     "    #true {cons (car lst) (flatten (cdr lst))}\n"
                     "}\n",
                     "defined: flatten");
    test_full_output(env, "flatten {}", "{}");
    test_full_output(env, "flatten {1}", "{1}");
    test_full_output(env, "flatten {1 2 3 4 5}", "{1 2 3 4 5}");
    test_full_output(env, "flatten {1 2 {3 4 5}}", "{1 2 3 4 5}");
    test_full_output(env, "flatten {{1 2} {3 4 5}}", "{1 2 3 4 5}");
    test_full_output(env, "flatten {{{1} 2} {3 {{4} 5}}}", "{1 2 3 4 5}");

    test_info_output(env,
                     "\n"
                     "fn {nth lst n} {\n"
                     "  if (== n 1)\n"
                     "    {car lst}\n"
                     "    {nth (cdr lst) (- n 1)}\n"
                     "}\n",
                     "defined: nth");
    test_number_output(env, "nth {1} 1", 1);
    test_number_output(env, "nth {1 2 3} 1", 1);
    test_number_output(env, "nth {1 2 3} 2", 2);
    test_number_output(env, "nth {1 2 3} 3", 3);
    test_full_output(env, "nth {{1} {2 3} 4 5 {6}} 5", "{6}");

    test_info_output(env,
                     "\n"
                     "fn {present? lst x} {\n"
                     "  cond\n"
                     "    (null? lst) {#false}\n"
                     "    (== (car lst) x) {#true}\n"
                     "    #true {present? (cdr lst) x}\n"
                     "}\n",
                     "defined: present");
    test_bool_output(env, "present? {1 2 3} 1", 1);
    test_bool_output(env, "present? {1 2 3} 3", 1);
    test_bool_output(env, "present? {1 2 3} 0", 0);
    test_bool_output(env, "present? {1 2 3} 3.14", 0);
    test_bool_output(env, "present? {1 2 3} +", 0);
    test_bool_output(env, "present? {+ {-} *} {-}", 1);
    test_bool_output(env, "present? {} 1", 0);

    test_info_output(env,
                     "\n"
                     "fn {last lst} {\n"
                     "  if (== (len lst) 1)\n"
                     "    {car lst}\n"
                     "    {last (cdr lst)}\n"
                     "}\n",
                     "defined: last");
    test_number_output(env, "last {1 2 3}", 3);
    test_number_output(env, "last {1}", 1);
    test_error_output(env, "last {}", "must be at least 1-long");
}

static void test_and(environment* env) {
    test_bool_output(env, "&& #true", 1);
    test_bool_output(env, "&& #false", 0);
    test_bool_output(env, "&& 1", 1);
    test_bool_output(env, "&& 0", 0);
    test_bool_output(env, "&& #true #true", 1);
    test_bool_output(env, "&& #false #true", 0);
    test_bool_output(env, "&& #true #false", 0);
    test_bool_output(env, "&& #false #false", 0);
    test_bool_output(env, "&& 1 1", 1);
    test_bool_output(env, "&& 0 1", 0);
    test_bool_output(env, "&& 1 0", 0);
    test_bool_output(env, "&& 0 0", 0);
    test_bool_output(env, "&& 1 1 1 1 1", 1);
    test_bool_output(env, "&& 0 1 1 1 1", 0);
    test_bool_output(env, "&& 0 1 0 1 1", 0);
    test_bool_output(env, "&& 1 1 1 1 0", 0);
    test_bool_output(env, "&& 0 0 0 0 0", 0);
    test_bool_output(env, "&& 0 (/ 1 0)", 0);

    test_error_output(env, "&& 1 (/ 1 0)", "division by zero");
    test_error_output(env, "&& + -", "can't cast function to bool");
}

static void test_or(environment* env) {
    test_bool_output(env, "|| #true", 1);
    test_bool_output(env, "|| #false", 0);
    test_bool_output(env, "|| 1", 1);
    test_bool_output(env, "|| 0", 0);
    test_bool_output(env, "|| #true #true", 1);
    test_bool_output(env, "|| #false #true", 1);
    test_bool_output(env, "|| #true #false", 1);
    test_bool_output(env, "|| #false #false", 0);
    test_bool_output(env, "|| 1 1", 1);
    test_bool_output(env, "|| 0 1", 1);
    test_bool_output(env, "|| 1 0", 1);
    test_bool_output(env, "|| 0 0", 0);
    test_bool_output(env, "|| 1 1 1 1 1", 1);
    test_bool_output(env, "|| 0 1 1 1 1", 1);
    test_bool_output(env, "|| 0 1 0 1 1", 1);
    test_bool_output(env, "|| 1 1 1 1 0", 1);
    test_bool_output(env, "|| 1 0 0 0 0", 1);
    test_bool_output(env, "|| 0 0 0 0 1", 1);
    test_bool_output(env, "|| 0 0 0 0 0", 0);
    test_bool_output(env, "|| 1 (/ 1 0)", 1);

    test_error_output(env, "|| 0 (/ 1 0)", "division by zero");
    test_error_output(env, "|| + -", "can't cast function to bool");
}

static void test_not(environment* env) {
    test_bool_output(env, "! #true", 0);
    test_bool_output(env, "! #false", 1);
    test_bool_output(env, "! 1", 0);
    test_bool_output(env, "! 0", 1);

    test_error_output(env, "! 0 1", "expects exactly 1 arg");
    test_error_output(env, "! (/ 1 0)", "division by zero");
    test_error_output(env, "! +", "can't cast function to bool");
}

void run_test() {
    counter = 0;

    RUN_TEST_FN(test_numeric);
    RUN_TEST_FN(test_errors);
    RUN_TEST_FN(test_full);
    RUN_TEST_FN(test_special);
    RUN_TEST_FN(test_string);
    RUN_TEST_FN(test_comment);

    RUN_TEST_FN(test_list);
    RUN_TEST_FN(test_first);
    RUN_TEST_FN(test_head);
    RUN_TEST_FN(test_tail);
    RUN_TEST_FN(test_join);
    RUN_TEST_FN(test_eval);
    RUN_TEST_FN(test_cons);
    RUN_TEST_FN(test_len);
    RUN_TEST_FN(test_init);

    RUN_TEST_FN(test_def);
    RUN_TEST_FN(test_lambda);
    RUN_TEST_FN(test_parent_env);
    RUN_TEST_FN(test_function_call);
    RUN_TEST_FN(test_fn);
    RUN_TEST_FN(test_del);

    RUN_TEST_FN(test_eq);
    RUN_TEST_FN(test_neq);
    RUN_TEST_FN(test_gt);
    RUN_TEST_FN(test_gte);
    RUN_TEST_FN(test_lt);
    RUN_TEST_FN(test_lte);
    RUN_TEST_FN(test_null_q);
    RUN_TEST_FN(test_zero_q);
    RUN_TEST_FN(test_list_q);

    RUN_TEST_FN(test_if);
    RUN_TEST_FN(test_cond);
    RUN_TEST_FN(test_recursion);

    RUN_TEST_FN(test_and);
    RUN_TEST_FN(test_or);
    RUN_TEST_FN(test_not);
}
