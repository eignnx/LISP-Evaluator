#ifndef BUILTINS_H
#define BUILTINS_H

#include "value_types.h"
#include "env_type.h"
#include "helper_macros.h"

ValueRef eval(ValueRef, Env*);

/// NOTE: `eval` ensures that `args` will be a `ListRef`

#define builtin_procedure_definition(NAME, IDENT, ...) \
    ValueRef IDENT##__builtin(ListRef args) {          \
        __VA_ARGS__                                    \
    }                                                  \
    static BuiltinProc IDENT = {                       \
        .name=NAME,                                    \
        .fn=IDENT##__builtin                           \
    }

#define special_form_definition(NAME, IDENT, ...)            \
    ValueRef IDENT##__special_form(ListRef args, Env* env) { \
        __VA_ARGS__                                          \
    }                                                        \
    static SpecialForm IDENT = {                             \
        .name=NAME,                                          \
        .fn=IDENT##__special_form                            \
    }

// Form: '(lambda (x1 x2 ...) body)
// Precondition: args = '((x1 x2 ...) body)
special_form_definition("lambda", lambda, {
    if (is_null(args))
        panic("%s", "Special form `lambda` takes 2 arguments, none given!");
    ListRef params = assume_list(car_lookup(args));
    PairRef body_pair = assume_pair_ref(cdr_lookup(args));
    ValueRef body = car_lookup(body_pair);
    if (!is_null(cdr_lookup(body_pair)))
        panic("%s", "Special form `lambda` expression can only handle one expression in the body!");
    return PROC(env, params, body);
});

// Form: '(set! symbol value)
// Precondition: args = '()
special_form_definition("set!", set_bang, {
    if (is_null(args))
        panic("%s", "Special form `set!` takes 2 arguments, none given!");
    SymbolRef symbol = assume_symbol_ref(car_lookup(args));
    PairRef rest = assume_pair_ref(cdr_lookup(args));
    ValueRef value = eval(car_lookup(rest), env);
    if (!is_null(cdr_lookup(rest)))
        panic("%s", "Special form `set!` takes no more than 2 arguments!");
    
    Env* env_ancestor = env_find(env, symbol);
    env_ancestor->value = value; // Mutate the environment.
    return (ValueRef) NULL;
});

builtin_procedure_definition("+", plus, {
    Number total = make_number(0);
    for (
        ListRef arg = args;
        !is_null(arg);
        arg = assume_list(cdr_lookup(arg))
    ) {
        Number x = assume_number(car_lookup(arg));
        total = NUM(GET_VALUE_DATA(total) + GET_VALUE_DATA(x));
    }
    return (ValueRef) total;
});

builtin_procedure_definition("*", times, {
    Number product = make_number(1);
    for (
        ListRef arg = args;
        !is_null(arg);
        arg = assume_list(cdr_lookup(arg))
    ) {
        Number x = assume_number(car_lookup(arg));
        product = NUM(GET_VALUE_DATA(product) * GET_VALUE_DATA(x));
    }
    return (ValueRef) product;
});

// '(cons car cdr)
builtin_procedure_definition("cons", cons, {
    if (is_null(args))
        panic("%s", "Builtin `cons` takes 2 arguments, none given!");
    ValueRef car = car_lookup(args);
    PairRef rest = assume_pair_ref(cdr_lookup(args));
    ValueRef cdr = car_lookup(rest);
    if (!is_null(cdr_lookup(rest)))
        panic("%s", "Builtin `cons` takes no more than 2 arguments!");
    return CONS(car, cdr);
});

// '(car)
builtin_procedure_definition("car", car, {
    if (is_null(args))
        panic("%s", "Builtin `car` takes 1 argument, none given!");
    ValueRef arg = car_lookup(args);
    if (!is_null(cdr_lookup(args)))
        panic("%s", "Builtin `car` takes no more than 1 argument!");
    PairRef pair = assume_pair_ref(arg);
    return car_lookup(pair);
});

// '(cdr arg)
builtin_procedure_definition("cdr", cdr, {
    if (is_null(args))
        panic("%s", "Builtin `cdr` takes 1 argument, none given!");
    ValueRef arg = car_lookup(args);
    if (!is_null(cdr_lookup(args)))
        panic("%s", "Builtin `cdr` takes no more than 1 argument!");
    PairRef pair = assume_pair_ref(arg);
    return cdr_lookup(pair);
});

static void register_builtin(Env** env, BuiltinProc proc) {
    char* name = proc.name;
    *env = make_env(*env, make_symbol_ref(name), make_builtin_proc(proc.name, proc.fn));
}

static void register_special_form(Env** env, SpecialForm form) {
    char* name = form.name;
    *env = make_env(*env, make_symbol_ref(name), make_special_form(form.name, form.fn));
}

Env* global_env() {
    Env* env = NULL;
    register_builtin(&env, plus);
    register_builtin(&env, times);
    register_builtin(&env, cons);
    register_builtin(&env, car);
    register_builtin(&env, cdr);
    register_special_form(&env, lambda);
    register_special_form(&env, set_bang);
    return env;
}

#endif