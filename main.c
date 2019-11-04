#define _GNU_SOURCE
#include <stdio.h> // printf, fprintf, fputc
#include <stdbool.h> // bool

#include "helper_macros.h"
#include "value_types.h"
#include "env_type.h"
#include "builtins.h"


Value apply(Value proc_val, Pair* args_val, Env* env);

bool self_evaluating(Value value) {
    return value == NULL
        || value->kind == NUMBER
        || value->kind == PROCEDURE
        || value->kind == BUILTIN_PROCEDURE
        ;
}

Value eval(Value expr, Env* env) {
    Pair* list;
    if (self_evaluating(expr)) {
        return expr;
    } else if (expr->kind == SYMBOL) {
        return env_lookup(env, (Symbol*) expr);
    } else if (expr->kind == PAIR && (list = (Pair*) expr, true)) {
        // General procedure application case.
        return apply(list->car, assume_list(list->cdr), env);
    }
    unimplemented();
}

// Note rad LISP-style recursion!
List* eval_args(List* args, Env* env) {
    if (args == NULL) {
        return NULL;
    } else {
        Value arg = eval(args->car, env);
        List* rest = eval_args(assume_list(args->cdr), env);
        return (List*) make_pair(arg, (Value) rest);
    }
}

Value apply_procedure(Proc*, List*, Env*);
Value apply_builtin_proc(BuiltinProc*, List*, Env*);
Value apply_special_form(SpecialForm*, List*, Env*);

Value apply(Value fn_unev, List* args_unev, Env* env) {
    // 1) Evaluate the procedure value in the current environment.
    Value fn = eval(fn_unev, env);

    if (fn == NULL) {
        panic("%s", "Cannot call null as a procedure!");
    }

    switch (fn->kind) {
    case PROCEDURE:
        return apply_procedure((Proc*) fn, args_unev, env);
    case BUILTIN_PROCEDURE:
        return apply_builtin_proc((BuiltinProc*) fn, args_unev, env);
    case SPECIAL_FORM:
        return apply_special_form((SpecialForm*) fn, args_unev, env);
    default:
        fprintf(stderr, "ERROR: unevaluated function = ");
        print_value(stderr, fn_unev);
        fprintf(stderr, ", unevaluated args = ");
        println_value(stderr, (Value) args_unev);
        panic("Cannot call value of type %s as a procedure!", typename_of(fn));
    }
}

Value apply_procedure(Proc* proc, List* args_unev, Env* env) {

    // 1) Evaluate the arguments in the current environment.
    // 2) Create a new environment--extending the procedure's creation environment--
    //    by pairing parameters with argument values.
    Env* new_env = proc->creation_env;
    Pair *unev_arg, *param;
    for (
        param = proc->params, unev_arg = args_unev;
        param != NULL && unev_arg != NULL;
        param = (Pair*) param->cdr, unev_arg = assume_list(unev_arg->cdr)
    ) {
        Value arg = eval(unev_arg->car, env);
        new_env = make_env(new_env, assume_symbol(param->car), arg);
    }

    // 3) Evaluate the procedure's body in the context of this new environment.
    return eval(proc->body, new_env);
}

Value apply_builtin_proc(BuiltinProc* proc, List* args_unev, Env* env) {
    List* args = eval_args(args_unev, env);
    return proc->fn(args);
}

Value apply_special_form(SpecialForm* proc, List* args_unev, Env* env) {
    return proc->fn(args_unev, env);
}

void test_lambda_application_and_builtins() {
    Env* e0 = global_env();
    Value lamb =
        LIST(SYM("lambda"), LIST(SYM("x"), SYM("y")),
            LIST(SYM("*"), SYM("x"),
                LIST(SYM("+"), SYM("y"), NUM(1))));
    
    println_value(stdout, lamb);
    Value program = LIST(lamb, NUM(2L), NUM(3));
    Value result = eval(program, e0);
    println_value(stdout, result);
}

void test_set_bang() {
    Env* e0 = global_env();
    Value program = LIST(SYM("set!"), SYM("lambda"), NUM(1337));
    Value result = eval(program, e0);
    print_env(stdout, e0);
    println_value(stdout, result);
}

void test_cons_car_cdr() {
    Env* env = global_env();
    // (cons 1 (cons 2 (cons 3 '())))
    Value program =
        LIST(SYM("cons"), NUM(1),
            LIST(SYM("cons"), NUM(2),
                LIST(SYM("cons"), NUM(3),
                    NULL)));
                    
    println_value(stdout, eval(LIST(SYM("car"), program), env));
    println_value(stdout, eval(LIST(SYM("cdr"), program), env));
}

int main(void) {
    test_lambda_application_and_builtins();
    test_set_bang();
    test_cons_car_cdr();
    return 0;
}
