#ifndef ENV_TYPE_H
#define ENV_TYPE_H

#include <stdlib.h> // malloc
#include "value_types.h"

typedef struct Env {
    struct Env* parent;
    SymbolRef symbol;
    ValueRef value;
} Env;

Env* env_find(Env* env, SymbolRef symbol) {
    for (; env != NULL; env = env->parent) {
        if (symbol_eq(env->symbol, symbol))
            return env;
    }
    panic("Unbound Symbol: `%s`", symbol_to_string(symbol));
}

ValueRef env_lookup(Env* env, SymbolRef symbol) {
    return env_find(env, symbol)->value;
}

void print_env(FILE* out, const Env* env) {
    fprintf(out, "Env {\n");
    for (; env != NULL; env = env->parent) {
        fprintf(out, "\t%s: ", symbol_to_string(env->symbol));
        print_value(out, env->value);
        fputc('\n', out);
    }
    fprintf(out, "}\n");
}

Env* make_env(Env* const parent, SymbolRef symbol, ValueRef value) {
    Env* env = malloc(sizeof(env));
    env->parent = parent;
    env->symbol = symbol;
    env->value = value;
    return env;
}

#endif