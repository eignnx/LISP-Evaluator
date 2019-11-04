#ifndef HELPER_MACROS_H
#define HELPER_MACROS_H

#include <stdio.h> // fprintf, stderr
#include <stdlib.h> // exit
#include <string.h> // strcmp
#include <stdbool.h> // false

#define panic(message, ...)                                 \
    fprintf(stderr, "panic[%s::%s:%d] " message "\n",       \
            __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__), \
    exit(1)

#define unreachable() \
    panic("%s", "Unreachable assumption violated!")

#define unimplemented() \
    panic("%s", "Unimplmented!")

#define HERE() \
    fprintf(stderr, "[%s::%s:%d] GOT HERE\n", __FILE__, __FUNCTION__, __LINE__)

#define value_downcast(value, TYPE, KIND)                                          \
    (((value) == NULL)                                                             \
        ? panic("Expected %s, got null!", #TYPE), (TYPE*) NULL :                   \
    (((value)->kind != KIND)                                                       \
        ? panic("Expected %s, got %s!", #TYPE, typename_of(value)), (TYPE*) NULL : \
    ((TYPE*) (value))))

#define nullable_value_downcast(value, TYPE, KIND)                                 \
    (((value) != NULL && (value)->kind != KIND)                                    \
        ? panic("Expected %s, got %s!", #TYPE, typename_of(value)), (TYPE*) NULL : \
    ((TYPE*) (value)))

#define builtin_procedure_definition(NAME, IDENT, ...) \
    Value* IDENT##__builtin(List* args) {              \
        __VA_ARGS__                                    \
    }                                                  \
    static BuiltinProc IDENT = {                       \
        .value=(Value) { .kind=BUILTIN_PROCEDURE },    \
        .name=NAME,                                    \
        .fn=IDENT##__builtin                           \
    }

#define special_form_definition(NAME, IDENT, ...)        \
    Value* IDENT##__special_form(List* args, Env* env) { \
        __VA_ARGS__                                      \
    }                                                    \
    static SpecialForm IDENT = {                         \
        .value=(Value) { .kind=SPECIAL_FORM },           \
        .name=NAME,                                      \
        .fn=IDENT##__special_form                        \
    }

#define str_eq(a, b) \
    (a == b || strcmp(a, b) == 0)

#endif