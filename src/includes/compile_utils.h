#ifndef TBASIC_COMPILE_UTILS_H
#define TBASIC_COMPILE_UTILS_H

#include "mystr.h"
#include "vec.h"

#define DEFAULT_SYMBOL_COUNT 8



STUB_SCALAR_VEC(int)

typedef enum symbol_domain_t : uint8_t {
    symbol_constant,
    symbol_local,
    symbol_func,
    symbol_string,
    symbol_upval,
} Domain;

typedef struct symbol_info_t {
    charspan name;
    int16_t id;
    Domain domain;
} SymbolInfo;

SymbolInfo make_symbol_info(charspan name_v, int16_t id, Domain d);

typedef struct symbol_table_t {
    SymbolInfo *infos;
    int length;
    int capacity;
    int16_t local_argc;     // ? count of parameter locals
    int16_t next_local_id;      // ? reused for local IDs
    int16_t next_global_id;     // ? reused for global / constant IDs
} SymbolTable;

SymbolTable make_symbol_table();
void SymbolTable_dud(SymbolTable *self);
void SymbolTable_del(SymbolTable *self);
void SymbolTable_copy(SymbolTable *dest, const SymbolTable *src);
const SymbolInfo *SymbolTable_find(const SymbolTable *symbols, const charspan *s, Domain d);
const SymbolInfo *SymbolTable_push(SymbolTable *symbols, const SymbolInfo *info);

STUB_VEC(SymbolTable)




// ? This ActiveLoop type stores information to help backpatch bytecode jumps in loops.
typedef struct active_loop_t {
    ScalarVec_int loop_breaks;
    ScalarVec_int loop_continues;
} ActiveLoop;

void ActiveLoop_dud(ActiveLoop *self);
void ActiveLoop_copy(ActiveLoop *self, const ActiveLoop *other);
void ActiveLoop_del(ActiveLoop *self);

STUB_VEC(ActiveLoop)


typedef enum bcgen_flag_t : uint8_t {
    cgen_dead = 0b00000000,
    cgen_visit_ok = 0b00000001,
    cgen_assign_to = 0b00000010,    // ? Is the compiler within a variable init / assignment's LHS?
    cgen_access_of = 0b00000100,    // ? Is the compiler within a member access expression LHS?
    cgen_lhs_local = 0b00001000,    // ? Has the compiler just consumed only an assignment LHS name?
    cgen_lhs_native = 0b00010000,   // ? Has the compiler consumed a native function's name in the LHS?
    cgen_lhs_upval = 0b00100000,    // ? Has the compiler just consumed an upval / captured name for an LHS?
    cgen_parse_err = 0b10000000
} CompHints;

static inline uint8_t compile_hints_check_flag(uint8_t bits, CompHints hint) {
    return 0 != (bits & hint);
}

#endif