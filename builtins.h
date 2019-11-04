#ifndef BUILTINS_H
#define BUILTINS_H

#include "value_types.h"
#include "env_type.h"
#include "helper_macros.h"

Value* eval(Value*, Env*);

// Form: '(lambda (x1 x2 ...) body)
// Precondition: args = '((x1 x2 ...) body)
special_form_definition("lambda", lambda, {
    if (args == NULL)
        panic("%s", "Special form `lambda` takes 2 arguments, none given!");
    List* params = assume_list(args->car);
    Pair* body_pair = assume_pair(args->cdr);
    Value* body = body_pair->car;
    if (body_pair->cdr != NULL)
        panic("%s", "Special form `lambda` expression can only handle one expression in the body!");
    return PROC(env, params, body);
});

// Form: '(set! symbol value)
// Precondition: args = '()
special_form_definition("set!", set_bang, {
    if (args == NULL)
        panic("%s", "Special form `set!` takes 2 arguments, none given!");
    Symbol* symbol = assume_symbol(args->car);
    Pair* rest = assume_pair(args->cdr);
    Value* value = eval(rest->car, env);
    if (rest->cdr != NULL)
        panic("%s", "Special form `set!` takes no more than 2 arguments!");
    
    Env* env_ancestor = env_find(env, symbol);
    env_ancestor->value = value; // Mutate the environment.
    return NULL;
});

builtin_procedure_definition("+", plus, {
    Number* total = make_number(0);
    for (
        List* arg = args;
        arg != NULL;
        arg = assume_list(arg->cdr)
    ) {
        Number* x = assume_number(arg->car);
        total->num += x->num;
    }
    return (Value*) total;
});

builtin_procedure_definition("*", times, {
    Number* product = make_number(1);
    for (
        List* arg = args;
        arg != NULL;
        arg = assume_list(arg->cdr)
    ) {
        Number* x = assume_number(arg->car);
        product->num *= x->num;
    }
    return (Value*) product;
});

// '(cons car cdr)
builtin_procedure_definition("cons", cons, {
    if (args == NULL)
        panic("%s", "Builtin `cons` takes 2 arguments, none given!");
    Value* car = args->car;
    Pair* rest = assume_pair(args->cdr);
    Value* cdr = rest->car;
    if (rest->cdr != NULL)
        panic("%s", "Builtin `cons` takes no more than 2 arguments!");
    return CONS(car, cdr);
});

// '(car)
builtin_procedure_definition("car", car, {
    if (args == NULL)
        panic("%s", "Builtin `car` takes 1 argument, none given!");
    Value* arg = args->car;
    if (args->cdr != NULL)
        panic("%s", "Builtin `car` takes no more than 1 argument!");
    Pair* pair = assume_pair(arg);
    return pair->car;
});

// '(cdr arg)
builtin_procedure_definition("cdr", cdr, {
    if (args == NULL)
        panic("%s", "Builtin `cdr` takes 1 argument, none given!");
    Value* arg = args->car;
    if (args->cdr != NULL)
        panic("%s", "Builtin `cdr` takes no more than 1 argument!");
    Pair* pair = assume_pair(arg);
    return pair->cdr;
});

static void register_builtin(Env** env, BuiltinProc* proc) {
    const char* name = proc->name;
    *env = make_env(*env, make_symbol(name), (Value*) proc);
}

static void register_special_form(Env** env, SpecialForm* form) {
    const char* name = form->name;
    *env = make_env(*env, make_symbol(name), (Value*) form);
}

Env* global_env() {
    Env* env = NULL;
    register_builtin(&env, &plus);
    register_builtin(&env, &times);
    register_builtin(&env, &cons);
    register_builtin(&env, &car);
    register_builtin(&env, &cdr);
    register_special_form(&env, &lambda);
    register_special_form(&env, &set_bang);
    return env;
}

#endif