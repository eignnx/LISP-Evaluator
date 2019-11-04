#ifndef VALUE_TYPES_H
#define VALUE_TYPES_H

#include <stdlib.h> // malloc
#include <stdint.h> // uint64_t

// Forward declaration. See `./env_type.h` for impl.
typedef struct Env Env;


typedef uint64_t ValueRef;

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
#define VALUE_DATA_MASK (~0UL >> VALUE_KIND_BITS)

#define GET_VALUE_KIND(ptr) ((unsigned) ((VALUE_KIND_MASK & (ptr)) >> (64 - VALUE_KIND_BITS)))
#define GET_VALUE_DATA(ptr) ((Idx) (VALUE_DATA_MASK & (ptr)))

#define is_null(value) ((value) == 0UL)
#define is_number(value) (GET_VALUE_KIND(value) == NUMBER)
#define is_symbol(value) (GET_VALUE_KIND(value) == SYMBOL)
#define is_pair(value) (GET_VALUE_KIND(value) == PAIR)
#define is_proc(value) (GET_VALUE_KIND(value) == PROCEDURE)
#define is_builtin_proc(value) (GET_VALUE_KIND(value) == BUILTIN_PROCEDURE)
#define is_special_form(value) (GET_VALUE_KIND(value) == SPECIAL_FORM)
#define is_other_value(value) (GET_VALUE_KIND(value) == OTHER_VALUE)

#define MAKE_VALUE(kind, data) \
    ((((uint64_t) kind) << (64 - VALUE_KIND_BITS)) | (VALUE_DATA_MASK & (data)))

typedef ValueRef Number;

typedef ValueRef SymbolRef;
typedef struct Symbol {
    char* str;
} Symbol;

/// NOTE: '() == NULL is not a Pair! Use ListRef instead.
typedef ValueRef PairRef;

// Semantically, a ListRef can be NULL, while a Pair should never be NULL.
typedef PairRef ListRef;
typedef struct Pair {
    ValueRef car;
    ValueRef cdr;
} Pair;

typedef ValueRef BigValueRef;
typedef struct BigValue {
    ValueRef v1;
    ValueRef v2;
    ValueRef v3;
} BigValue;

typedef ValueRef ProcRef;
typedef struct Proc {
    Env* creation_env;
    PairRef params;
    ValueRef body;
} Proc;

typedef ValueRef (*BuiltinFnPtr)(ListRef args);
typedef ValueRef BuiltinProcRef;
typedef struct BuiltinProc {
    char* name;
    BuiltinFnPtr fn;
} BuiltinProc;

typedef ValueRef (*SpecialFormFnPtr)(ListRef args, Env* env);
typedef ValueRef SpecialFormRef;
typedef struct SpecialForm {
    char* name;
    SpecialFormFnPtr fn;
} SpecialForm;

////////////////////ALLOCATION POOLS////////////////
#define MAX_ALLOC_SIZE (1UL << (64 - VALUE_KIND_BITS))
#define ALLOC_SIZE (MAX_ALLOC_SIZE >> 40) // dont use up all of memory

static struct {
    ValueRef cars[ALLOC_SIZE];
    ValueRef cdrs[ALLOC_SIZE];
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
    ValueRef v1[ALLOC_SIZE];
    ValueRef v2[ALLOC_SIZE];
    ValueRef v3[ALLOC_SIZE];
    Idx next_idx;
} BIG_VALUES = {
    .next_idx=0UL
};
////////////////////////////////////////////////////

ValueRef car_lookup(PairRef pair) {
    Idx idx = GET_VALUE_DATA(pair);
    if (idx < PAIRS.next_idx) {
        return PAIRS.cars[idx];
    } else {
        panic("Car index '%lu' out of bounds!", idx);
    }
}

ValueRef cdr_lookup(PairRef pair) {
    Idx idx = GET_VALUE_DATA(pair);
    if (idx < PAIRS.next_idx) {
        return PAIRS.cdrs[idx];
    } else {
        panic("Cdr index '%lu' out of bounds!", idx);
    }
}

PairRef make_pair_ref(ValueRef car, ValueRef cdr) {
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
    if (idx >= SYMBOLS.next_idx) panic("Symbol index '%lu' out of bounds!", idx);
    return SYMBOLS.symbols[idx];
}

BigValue big_value_lookup(BigValueRef val) {
    Idx idx = GET_VALUE_DATA(val);
    if (idx >= BIG_VALUES.next_idx) panic("Big value index '%lu' out of bounds!", idx);
    return (BigValue) {
        .v1=BIG_VALUES.v1[idx],
        .v2=BIG_VALUES.v2[idx],
        .v3=BIG_VALUES.v3[idx],
    };
}

const char* typename_of(ValueRef value) {
    switch (GET_VALUE_KIND(value)) {
    case NULL_LIST: return "null";
    case NUMBER: return "number";
    case SYMBOL: return "symbol";
    case PAIR: return "pair";
    case PROCEDURE: return "procedure";
    case BUILTIN_PROCEDURE: return "builtin procedure";
    case SPECIAL_FORM: return "special form";
    default: unimplemented();
    }
}

#define SYM(s) ((ValueRef) make_symbol_ref(s))

bool symbol_eq(const SymbolRef a, const SymbolRef b) {
    return a == b;
}

char* symbol_to_string(const SymbolRef sym) {
    return symbol_lookup(sym).str;
}

// TODO: check that it's within bounds!
Number make_number(int64_t num) {
    return MAKE_VALUE(NUMBER, num);
}

#define NUM(n) ((ValueRef) make_number(n))


#define CONS(x, y) ((ValueRef) make_pair_ref(x, y))

ValueRef make_list(ValueRef values[], size_t count) {
    ValueRef list = (ValueRef) NULL;
    for (int i = count-1; i >= 0; i--) {
        list = make_pair_ref(values[i], list);
    }
    return list;
}

#define LIST(...)                                          \
    make_list(                                             \
        (ValueRef[]) { __VA_ARGS__ },                         \
        sizeof((ValueRef[]) { __VA_ARGS__ }) / sizeof(ValueRef)  \
    )

bool value_eq(ValueRef, ValueRef);

/// Expects non-null arguments.
bool pair_eq(PairRef a, PairRef b) {
    return value_eq(car_lookup(a), car_lookup(b)) && value_eq(cdr_lookup(a), cdr_lookup(b));
}

bool value_eq(ValueRef a, ValueRef b) {
    if (is_null(a) && is_null(b)) return true;
    else if (is_null(a) || is_null(b)) return false;
    if (GET_VALUE_KIND(a) != GET_VALUE_KIND(b)) return false;
    switch (GET_VALUE_KIND(a)) {
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

Proc proc_lookup(ProcRef proc) {
    BigValue bv = big_value_lookup((BigValueRef) proc);
    return (Proc) {
        .creation_env = (Env*) bv.v1,
        .params = (PairRef) bv.v2,
        .body = (ValueRef) bv.v3,
    };
}

ProcRef make_proc(Env* creation_env, PairRef params, ValueRef body) {
    Idx idx = BIG_VALUES.next_idx++;
    if (idx >= ALLOC_SIZE) panic("%s", "BIG_VALUES alloc overflow!");
    BIG_VALUES.v1[idx] = (ValueRef) creation_env;
    BIG_VALUES.v2[idx] = (ValueRef) params;
    BIG_VALUES.v3[idx] = (ValueRef) body;
    return MAKE_VALUE(PROCEDURE, idx);
}

#define PROC(env, formals, body) ((ValueRef) make_proc(env, formals, body))

Pair pair_lookup(PairRef pair) {
    Idx idx = GET_VALUE_DATA(pair);
    if (idx >= PAIRS.next_idx) panic("Pair idx '%lu' out of bounds!", idx);
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
    PAIRS.cars[idx] = (ValueRef) name;
    PAIRS.cdrs[idx] = (ValueRef) fn;
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
    PAIRS.cars[idx] = (ValueRef) name;
    PAIRS.cdrs[idx] = (ValueRef) fn;
    return MAKE_VALUE(SPECIAL_FORM, idx);
}

/////////////////////////////////// ASSUME /////////////////////////////////////////////
#define value_downcast(value, TYPE, KIND)                                        \
    ((GET_VALUE_KIND(value) != KIND)                                             \
        ? panic("Expected %s, got %s!", #TYPE, typename_of(value)), (TYPE) NULL \
        : ((TYPE) (value)))

#define nullable_value_downcast(value, TYPE, KIND)                              \
    ((!is_null(value) && GET_VALUE_KIND(value) != KIND)                         \
        ? panic("Expected %s, got %s!", #TYPE, typename_of(value)), (TYPE) NULL \
        : ((TYPE) (value)))

//==============================================================================

#define assume_number(value) value_downcast(value, Number, NUMBER)
#define assume_symbol_ref(value) value_downcast(value, SymbolRef, SYMBOL)
#define assume_pair_ref(value) value_downcast(value, PairRef, PAIR)
#define assume_list(value) nullable_value_downcast(value, ListRef, PAIR)
#define assume_proc_ref(value) value_downcast(value, ProcRef, PROCEDURE)
#define assume_builtinproc_ref(value) value_downcast(value, BuiltinProcRef, BUILTIN_PROCEDURE)
#define assume_specialform_ref(value) value_downcast(value, SpecialFormRef, SPECIAL_FORM)
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////// PRINTING ///////////////////////////////////
void print_value(FILE*, const ValueRef);

/// Assumes non-null pair pointer.
void print_pair(FILE* out, const PairRef pair) {
    fprintf(out, "(");
    ValueRef cdr;
    for (
        cdr = (ValueRef) pair;
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

    if (is_null(cdr)) {
        fputc(')', out);
    } else {
        fprintf(out, ". ");
        print_value(out, cdr);
        fprintf(out, ")");
    }
}

void print_value(FILE* out, ValueRef value) {
    switch (GET_VALUE_KIND(value)) {
    case NULL_LIST:
        fprintf(out, "%s", "'()");
        break;
    case NUMBER:
        fprintf(out, "%lu", GET_VALUE_DATA(value));
        break;
    case SYMBOL:
        fprintf(out, "%s", symbol_to_string((SymbolRef) value));
        break;
    case PAIR:
        print_pair(out, (PairRef) value);
        break;
    case PROCEDURE:
        // Print the index.
        fprintf(out, "<procedure[%lu]>", GET_VALUE_DATA(value));
        break;
    case BUILTIN_PROCEDURE:
        fprintf(out, "<builtin-procedure[%s]>", builtin_proc_lookup(value).name);
        break;
    case SPECIAL_FORM:
        fprintf(out, "<special-form[%s]>", special_form_lookup(value).name);
        break;
    default:
        panic("`print_value` is not implemented for value kind: %u", GET_VALUE_KIND(value));
    }
}

void println_value(FILE* out, ValueRef value) {
    print_value(out, value);
    fputc('\n', out);
}
///////////////////////////////////////////////////////////////////

#endif