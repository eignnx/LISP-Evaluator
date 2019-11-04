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

#define DBG_REF(R) \
    fprintf(stderr, "[%s::%s:%d] %s == %#lx\n", __FILE__, __FUNCTION__, __LINE__, #R, R)

#define str_eq(a, b) \
    (a == b || strcmp(a, b) == 0)

#define ASSERT_VALUE_REFS_EQ(A, B) \
    ((!value_eq(A, B)) ? \
        fprintf(stderr, "panic[%s::%s:%d] ASSERTION FAILURE: ",       \
                __FILE__, __FUNCTION__, __LINE__), \
        print_value(stderr, A), \
        fprintf(stderr, " != "), \
        println_value(stderr, B), \
        exit(1) \
    : 1 )

#endif