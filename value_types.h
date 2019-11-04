#ifndef VALUE_TYPES_H
#define VALUE_TYPES_H

#include <stdlib.h> // malloc
#include <stdint.h> // int64_t

// Forward declaration. See `./env_type.h` for impl.
typedef struct Env Env;


typedef void* Value;

typedef uint64_t Idx;

// Must fit in 3 bits.
#define VALUE_KIND_BITS 3
enum VALUE_KIND {
    NULL_LIST = 0,
    NUMBER = 1,
    SYMBOL = 2,
    PAIR = 3,
    PROCEDURE = 4,
    BUILTIN_PROCEDURE = 5,
    SPECIAL_FORM = 6,
    OTHER_VALUE = 7, // Everything other than the above types
};

#define VALUE_KIND_MASK (~0UL << (64 - VALUE_KIND_BITS))
#define VALUE_DATA_MASK (~0UL << VALUE_KIND_BITS)

#define GET_VALUE_KIND(ptr) ((VALUE_KIND_MASK & (ptr)) >> (64 - VALUE_KIND_BITS))
#define GET_VALUE_DATA(ptr) ((Idx) (VALUE_DATA_MASK & (ptr)))

#define is_null(value) ((value) == 0UL)
#define is_number(value) (GET_VALUE_KIND(value) == NUMBER)
#define is_symbol(value) (GET_VALUE_KIND(value) == SYMBOL)
#define is_pair(value) (GET_VALUE_KIND(value) == PAIR)
#define is_proc(value) (GET_VALUE_KIND(value) == PROCEDURE)
#define is_builtin_proc(value) (GET_VALUE_KIND(value) == BUILTIN_PROCEDURE)
#define is_special_form(value) (GET_VALUE_KIND(value) == SPECIAL_FORM)
#define is_other_value(value) (GET_VALUE_KIND(value) == OTHER_VALUE)

#define MAKE_VALUE(kind, data)            \
    (((kind) << (64 - VALUE_KIND_BITS))   \
    | (VALUE_DATA_MASK & (data)))

typedef struct Number {
    int64_t num;
} Number;

typedef Value SymbolRef;
typedef struct Symbol {
    char* str;
} Symbol;

/// NOTE: '() == NULL is not a Pair! Use List instead.
typedef Value PairRef;

// Semantically, a List can be NULL, while a Pair should never be NULL.
typedef Pair List;

typedef Value BigValueRef;
typedef struct BigValue {
    Value v1;
    Value v2;
    Value v3;
} BigValue;

typedef Value ProcRef;
typedef struct Proc {
    Env* creation_env;
    PairRef params;
    Value body;
} Proc;

typedef Value (*BuiltinFnPtr)(List args);
typedef Value BuiltinProcRef;
typedef struct BuiltinProc {
    const char* name;
    BuiltinFnPtr fn;
} BuiltinProc;

typedef Value (*SpecialFormFnPtr)(List args, Env* env);
typedef Value SpecialFormRef;
typedef struct SpecialForm {
    const char* name;
    SpecialFormFnPtr fn;
} SpecialForm;

////////////////////ALLOCATION POOLS////////////////
#define MAX_ALLOC_SIZE (1 << (64 - VALUE_TAG_BITS))
#define ALLOC_SIZE (MAX_ALLOC_SIZE >> 40) // dont use up all of memory

static struct {
    Value cars[ALLOC_SIZE];
    Value cdrs[ALLOC_SIZE];
    Idx next_idx;
} PAIRS = {
    .next_idx=0UL
};

static struct {
    Symbol symbols[ALLOC_SIZE];
    Idx next_idx;
} SYMBOLS = {
    .next_idx=0UL
};

static struct {
    Value v1[ALLOC_SIZE];
    Value v2[ALLOC_SIZE];
    Value v3[ALLOC_SIZE];
    Idx next_idx;
} BIG_VALUES = {
    .next_idx=0UL
};
////////////////////////////////////////////////////

Value car_lookup(PairRef pair) {
    Idx idx = GET_VALUE_DATA(pair);
    if (idx < PAIRS.next_idx) {
        return PAIRS.cars[idx];
    } else {
        panic("Car index '%ld' out of bounds!", idx);
    }
}

Value cdr_lookup(PairRef pair) {
    Idx idx = GET_VALUE_DATA(pair);
    if (idx < PAIRS.next_idx) {
        return PAIRS.cdrs[idx];
    } else {
        panic("Cdr index '%ld' out of bounds!", idx);
    }
}

PairRef make_pair_ref(Value car, Value cdr) {
    Idx idx = PAIRS.next_idx++;
    if (idx > ALLOC_SIZE) panic("%s", "PAIRS alloc error!");
    PAIRS.cars[idx] = car;
    PAIRS.cdrs[idx] = cdr;
    return (PairRef) MAKE_VALUE(PAIR, idx);
}

// Interns a string!
SymbolRef make_symbol_ref(char* str) {
    for (Idx idx = 0; idx < SYMBOLS.next_idx; idx++) {
        if (str_eq(str, SYMBOLS.symbols[idx].str)) {
            return MAKE_VALUE(SYMBOL, idx);
        }
    }
    Idx idx = SYMBOLS.next_idx++;
    if (idx >= ALLOC_SIZE) panic("%s", "SYMBOLS alloc error!");
    SYMBOLS.symbols[idx] = (Symbol) { .str=str };
    return MAKE_VALUE(SYMBOL, idx);
}

Symbol symbol_lookup(SymbolRef sym) {
    Idx idx = GET_VALUE_DATA(sym);
    if (idx >= SYMBOLS.next_idx) panic("Symbol index '%ld' out of bounds!", idx);
    return SYMBOLS.symbols[idx];
}

BigValue big_value_lookup(BigValueRef val) {
    Idx idx = GET_VALUE_DATA(val);
    if (idx >= BIG_VALUES.next_idx) panic("Big value index '%ld' out of bounds!", idx);
    return (BigValue) {
        .v1=BIG_VALUES.v1[idx],
        .v2=BIG_VALUES.v2[idx],
        .v3=BIG_VALUES.v3[idx],
    };
}

const char* typename_of(Value value) {
    if (value == NULL) return "null";
    switch (GET_VALUE_KIND(value)) {
    case NUMBER: return "number";
    case SYMBOL: return "symbol";
    case PAIR: return "pair";
    case PROCEDURE: return "procedure";
    case BUILTIN_PROCEDURE: return "builtin procedure";
    case SPECIAL_FORM: return "special form";
    default: unimplemented();
    }
}

#define SYM(s) ((Value) make_symbol_ref(s))

bool symbol_eq(const SymbolRef a, const SymbolRef b) {
    return a == b;
}

char* symbol_to_string(const SymbolRef sym) {
    return symbol_lookup(sym).str;
}

// TODO: check that it's within bounds!
Number make_number(int64_t num) {
    MAKE_VALUE(NUMBER, num);
}

#define NUM(n) ((Value) make_number(n))


#define CONS(x, y) ((Value) make_pair_ref(x, y))

Value make_list(Value values[], size_t count) {
    Value list = NULL;
    for (int i = count-1; i >= 0; i--) {
        list = make_pair_ref(values[i], list);
    }
    return list;
}

#define LIST(...)                                          \
    make_list(                                             \
        (Value[]) { __VA_ARGS__ },                         \
        sizeof((Value[]) { __VA_ARGS__ }) / sizeof(Value)  \
    )

bool value_eq(Value, Value);

/// Expects non-null arguments.
bool pair_eq(Pair* a, Pair* b) {
    return value_eq(a->car, b->car) && value_eq(a->cdr, b->cdr);
}

bool value_eq(Value a, Value b) {
    if (a == NULL && b == NULL) return true;
    else if (a == NULL || b == NULL) return false;
    if (a->kind != b->kind) return false;
    switch (a->kind) {
    case SYMBOL: {
        return symbol_eq((SymbolRef) a, (SymbolRef) b);
    }
    case PAIR: {
        return pair_eq((PairRef) a, (PairRef) b);
    }
    case NUMBER:            // Bit-for-bit equality
    case PROCEDURE:         // Pointer equality
    case BUILTIN_PROCEDURE: // Pointer equality
    case SPECIAL_FORM:      // Pointer equality
        return a == b;
    default:
        unimplemented();
    }
}

void proc_lookup(ProcRef proc) {
    BigValue bv = big_value_lookup((BigValueRef) proc);
    return (Proc) {
        .creation_env = (Env*) bv.v1,
        .params = (PairRef) bv.v2,
        .body = (Value) bv.v3,
    };
}

ProcRef make_proc(Env* creation_env, PairRef params, Value body) {
    Idx idx = BIG_VALUES.next_idx++;
    if (idx >= ALLOC_SIZE) panic("%s", "BIG_VALUES alloc overflow!");
    BIG_VALUES.v1[idx] = (Value) creation_env;
    BIG_VALUES.v2[idx] = (Value) params;
    BIG_VALUES.v3[idx] = (Value) body;
    return MAKE_VALUE(PROCEDURE, idx);
}

#define PROC(env, formals, body) ((Value) make_proc(env, formals, body))

Pair pair_lookup(PairRef pair) {
    Idx idx = GET_VALUE_DATA(pair);
    if (idx >= PAIRS.next_idx) panic("Pair idx '%ld' out of bounds!", idx);
    return (Pair) {
        .car=PAIRS.cars[idx],
        .cdr=PAIRS.cdrs[idx],
    };
}

BuiltinProc builtin_proc_lookup(BuiltinProcRef proc) {
    Pair pair = pair_lookup((PairRef) proc);
    return (BuiltinProc) {
        .name = (char*) pair.car,
        .fn = (BuiltinFnPtr) pair.cdr,
    };
}

BuiltinProcRef make_builtin_proc(const char* name, BuiltinFnPtr fn) {
    Idx idx = PAIRS.next_idx++;
    if (idx >= ALLOC_SIZE) panic("%s", "PAIRS alloc overflow");
    PAIRS.cars[idx] = (Value) name;
    PAIRS.cdrs[idx] = (Value) fn;
    return MAKE_VALUE(BUILTIN_PROCEDURE, idx);
}

SpecialForm special_form_lookup(SpecialFormRef form) {
    Pair pair = pair_lookup((PairRef) form);
    return (SpecialForm) {
        .name = (char*) pair.car,
        .fn = (SpecialFormFnPtr) pair.cdr,
    };
}

SpecialFormRef make_special_form(char* name, SpecialFormFnPtr fn) {
    Idx idx = PAIRS.next_idx++;
    if (idx >= ALLOC_SIZE) panic("%s", "PAIRS alloc overflow");
    PAIRS.cars[idx] = (Value) name;
    PAIRS.cdrs[idx] = (Value) fn;
    return MAKE_VALUE(SPECIAL_FORM, idx);
}


////////////////////////// ASSUME /////////////////////////////////////////
#define assume_number(value) value_downcast(value, Number, NUMBER)
#define assume_symbol(value) value_downcast(value, Symbol, SYMBOL)
#define assume_pair(value) value_downcast(value, Pair, PAIR)
#define assume_list(value) nullable_value_downcast(value, List, PAIR)
#define assume_proc(value) value_downcast(value, Proc, PROCEDURE)
#define assume_builtinproc(value) value_downcast(value, BuiltinProc, BUILTIN_PROCEDURE)
#define assume_specialform(value) value_downcast(value, SpecialForm, SPECIAL_FORM)
//////////////////////////////////////////////////////////////////////////

/////////////////////////// PRINTING ///////////////////////////////////
void print_value(FILE*, const Value);

/// Assumes non-null pair pointer.
void print_pair(FILE* out, const PairRef pair) {
    fprintf(out, "(");
    Value cdr;
    for (
        cdr = (Value) pair;
        is_pair(cdr);
        cdr = cdr_lookup((PairRef) cdr)
    ) {
        // Safe to assume cdr is a pair.
        PairRef cdr_pair = (PairRef) cdr;
        print_value(out, car_lookup(cdr_pair));
        if (GET_VALUE_KIND(cdr_lookup(cdr_pair)) == PAIR) {
            // Don't print space for last element.
            fputc(' ', out);
        }
    }

    if (cdr == NULL) {
        fputc(')', out);
    } else {
        fprintf(out, ". ");
        print_value(out, cdr);
        fprintf(out, ")");
    }
}

void print_value(FILE* out, Value value) {
    switch (GET_VALUE_KIND(value)) {
    case NULL_LIST:
        fprintf(out, "%s", "'()");
        break;
    case NUMBER:
        fprintf(out, "%ld", GET_VALUE_DATA(value));
        break;
    case SYMBOL:
        fprintf(out, "%s", symbol_to_string((SymbolRef) value));
        break;
    case PAIR:
        print_pair(out, (PairRef) value);
        break;
    case PROCEDURE:
        // Print the index.
        fprintf(out, "<procedure[%ld]>", GET_VALUE_DATA(value));
        break;
    case BUILTIN_PROCEDURE:
        fprintf(out, "<builtin-procedure[%s]>", builtin_proc_lookup(value).name);
        break;
    case SPECIAL_FORM:
        fprintf(out, "<special-form[%s]>", special_form_lookup(value).name);
        break;
    default:
        panic("`print_value` is not implemented for value kind: %d", GET_VALUE_KIND(value));
    }
}

void println_value(FILE* out, Value value) {
    print_value(out, value);
    fputc('\n', out);
}
///////////////////////////////////////////////////////////////////

#endif