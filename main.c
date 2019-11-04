#define _GNU_SOURCE
#include <stdio.h> // printf, fprintf, fputc
#include <stdbool.h> // bool

#include "helper_macros.h"
#include "value_types.h"
#include "env_type.h"
#include "builtins.h"


ValueRef apply(ValueRef proc_val, PairRef args_val, Env* env);

bool self_evaluating(ValueRef value) {
    switch (GET_VALUE_KIND(value)) {
    case NULL_LIST:
    case NUMBER:
    case PROCEDURE:
    case BUILTIN_PROCEDURE:
    case SPECIAL_FORM:
        return true;
    default:
        return false;
    }
}

ValueRef eval(ValueRef expr, Env* env) {
    PairRef list;
    if (self_evaluating(expr)) {
        return expr;
    } else if (is_symbol(expr)) {
        return env_lookup(env, (SymbolRef) expr);
    } else if (is_pair(expr) && (list = (PairRef) expr, true)) {
        // General procedure application case.
        return apply(car_lookup(list), assume_list(cdr_lookup(list)), env);
    } else {
        unimplemented();
    }
}

// Note rad LISP-style recursion!
ListRef eval_args(ListRef args, Env* env) {
    if (is_null(args)) {
        return (ListRef) NULL;
    } else {
        ValueRef arg = eval(car_lookup(args), env);
        ListRef rest = eval_args(assume_list(cdr_lookup(args)), env);
        return (ListRef) make_pair_ref(arg, (ValueRef) rest);
    }
}

ValueRef apply_procedure(Proc, ListRef, Env*);
ValueRef apply_builtin_proc(BuiltinProc, ListRef, Env*);
ValueRef apply_special_form(SpecialForm, ListRef, Env*);

ValueRef apply(ValueRef fn_unev, ListRef args_unev, Env* env) {
    // 1) Evaluate the procedure value in the current environment.
    ValueRef fn = eval(fn_unev, env);

    if (is_null(fn)) {
        panic("%s", "Cannot call null as a procedure!");
    }

    switch (GET_VALUE_KIND(fn)) {
    case PROCEDURE:
        return apply_procedure(proc_lookup(fn), args_unev, env);
    case BUILTIN_PROCEDURE:
        return apply_builtin_proc(builtin_proc_lookup(fn), args_unev, env);
    case SPECIAL_FORM:
        return apply_special_form(special_form_lookup(fn), args_unev, env);
    default:
        fprintf(stderr, "ERROR: unevaluated function = ");
        print_value(stderr, fn_unev);
        fprintf(stderr, ", unevaluated args = ");
        println_value(stderr, (ValueRef) args_unev);
        panic("Cannot call value of type %s as a procedure!", typename_of(fn));
    }
}

ValueRef apply_procedure(Proc proc, ListRef args_unev, Env* env) {

    // 1) Evaluate the arguments in the current environment.
    // 2) Create a new environment--extending the procedure's creation environment--
    //    by pairing parameters with argument values.
    Env* new_env = proc.creation_env;
    PairRef unev_arg, param;
    for (
        param = proc.params, unev_arg = args_unev;
        !is_null(param)&& !is_null(unev_arg);
        param = (PairRef) cdr_lookup(param), unev_arg = assume_list(cdr_lookup(unev_arg))
    ) {
        ValueRef arg = eval(car_lookup(unev_arg), env);
        new_env = make_env(new_env, assume_symbol_ref(car_lookup(param)), arg);
    }

    // 3) Evaluate the procedure's body in the context of this new environment.
    return eval(proc.body, new_env);
}

ValueRef apply_builtin_proc(BuiltinProc proc, ListRef args_unev, Env* env) {
    ListRef args = eval_args(args_unev, env);
    return proc.fn(args);
}

ValueRef apply_special_form(SpecialForm form, ListRef args_unev, Env* env) {
    return form.fn(args_unev, env);
}

void test_lambda_application_and_builtins() {
    Env* e0 = global_env();
    ValueRef lamb =
        LIST(SYM("lambda"), LIST(SYM("x"), SYM("y")),
            LIST(SYM("*"), SYM("x"),
                LIST(SYM("+"), SYM("y"), NUM(1))));
    
    ValueRef program = LIST(lamb, NUM(2), NUM(3));
    ValueRef result = eval(program, e0);
    ASSERT_VALUE_REFS_EQ(result, NUM(8));
}

void test_set_bang() {
    Env* e0 = global_env();
    ValueRef program = LIST(SYM("set!"), SYM("lambda"), NUM(1337));
    ValueRef result = eval(program, e0);
    ASSERT_VALUE_REFS_EQ(env_lookup(e0, SYM("lambda")), NUM(1337));
}

void test_cons_car_cdr() {
    Env* env = global_env();
    // (cons 1 (cons 2 (cons 3 '())))
    ValueRef program =
        LIST(SYM("cons"), NUM(1),
            LIST(SYM("cons"), NUM(2),
                LIST(SYM("cons"), NUM(3),
                    (ValueRef) NULL)));
                    
    ASSERT_VALUE_REFS_EQ(eval(LIST(SYM("car"), program), env), NUM(1));
    ASSERT_VALUE_REFS_EQ(eval(LIST(SYM("cdr"), program), env), LIST(NUM(2), NUM(3)));
}

void test() {
    test_lambda_application_and_builtins();
    test_set_bang();
    test_cons_car_cdr();
    printf("All tests passed!\n");
}

int main(void) {
    test();
    return 0;
}
