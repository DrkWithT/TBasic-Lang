#ifndef TBASIC_COMPILER_H
#define TBASIC_COMPILER_H

#include "mystr.h"
#include "lex.h"
#include "bytecode.h"



#define DEFAULT_SYMBOL_CAPACITY 8

STUB_SCALAR_VEC(int)



typedef enum symbol_domain_t : uint8_t {
    symbol_constant,
    symbol_local,
    symbol_func,
    symbol_native,
    symbol_string,
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
    int var_alloc_ip;       // ? marks where a RESERVE (n) instruction is
    int16_t local_argc;     // ? count of parameter locals
    int16_t next_local_id;      // ? reused for local IDs
    int16_t next_global_id;     // ? reused for global / constant IDs
} SymbolTable;

SymbolTable make_symbol_table();
void symbol_table_del(SymbolTable *self);
void symbol_table_clear(SymbolTable *self);
const SymbolInfo *symbol_table_find(const SymbolTable *symbols, const charspan *s, Domain d);
const SymbolInfo *symbol_table_push(SymbolTable *symbols, const SymbolInfo *info);



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
    cgen_no_flags = 0b0000,
    cgen_assign_to = 0b0001,    // ? Is the compiler within a variable init / assignment's LHS?
    cgen_access_of = 0b0010,    // ? Is the compiler within a member access expression LHS?
    cgen_lhs_local = 0b0100,    // ? Has the compiler just consumed only an assignment LHS name?
    cgen_lhs_native = 0b1000    // ? Has the compiler consumed a native function's name in the LHS?
} CodegenFlag;



typedef struct compiler_t {
    SymbolTable globals;
    SymbolTable locals;
    AnyVec_ActiveLoop loops;
    Token prev;
    Token curr;
    int errors;
    int16_t chunk_idx;  // ? 0 indexes top level code, 1+ indexes a code chunk per procedure, applying only for compiling a FUN decl.
    int16_t next_native_id;
    int16_t next_str_id;
    uint8_t saved_id;
    uint8_t flags;
} Compiler;

Compiler make_compiler();
void compiler_del(Compiler *self);

void compiler_map_native(Compiler *self, const charspan *s);

int8_t compiler_peek_past_spaces(const Compiler *self, Lexer *l, const charspan *source, char c);
int8_t compiler_match_curr(const Compiler *self, TkTag tag);
int8_t compiler_match_prev(const Compiler *self, TkTag tag);
Token compiler_advance_tk(Compiler *self, Lexer *lexer, const charspan *s);
void compiler_eat_tk(Compiler *self, Lexer *lexer, const charspan *s);
void compiler_warn(Compiler *self, const char *msg, const Token *tk, const charspan *s);

void compiler_flag_on(Compiler *self, CodegenFlag flag);
void compiler_flag_off(Compiler *self, CodegenFlag flag);
int8_t compiler_flag_of(const Compiler *self, CodegenFlag flag);

size_t compiler_emit_op(Compiler *self, Program *pg, Opcode op);
size_t compiler_emit_op_unflagged(Compiler *self, Program *pg, Opcode op, int16_t wide);
size_t compiler_emit_op_flagged(Compiler *self, Program *pg, Opcode op, uint8_t flags, int16_t wide);

void compiler_patch_reserve_inst(Compiler *self, const SymbolTable *scope, Program *pg);

const SymbolInfo *compiler_resolve_name(const Compiler *self, const charspan *s);
const SymbolInfo *compiler_record_function(Compiler *self, Program *pg, const charspan *s, int chunk_id);
const SymbolInfo *compiler_record_local(Compiler *self, Program *pg, const charspan *s);
const SymbolInfo *compiler_record_constant(Compiler *self, Program *pg, const charspan *s_symbol, Value v);
const SymbolInfo *compiler_record_string(Compiler *self, Program *pg, const charspan *s);

ActiveLoop *compiler_enter_loop(Compiler *self);
void compiler_leave_loop(Compiler *self);
void compiler_track_break_pos(Compiler *self, int pos);
void compiler_track_continue_pos(Compiler *self, int pos);

int8_t compiler_do_list(Compiler *self, Lexer *lexer, const charspan *s, Program *pg);
int8_t compiler_do_dict(Compiler *self, Lexer *lexer, const charspan *s, Program *pg);
int8_t compiler_do_literal(Compiler *self, Lexer *lexer, const charspan *s, Program *pg);
int8_t compiler_do_lhs(Compiler *self, Lexer *lexer, const charspan *s, Program *pg);
int8_t compiler_do_call(Compiler *self, Lexer *lexer, const charspan *s, Program *pg);
int8_t compiler_do_unary(Compiler *self, Lexer *lexer, const charspan *s, Program *pg);
int8_t compiler_do_null_coal(Compiler *self, Lexer *lexer, const charspan *s, Program *pg);
int8_t compiler_do_factor(Compiler *self, Lexer *lexer, const charspan *s, Program *pg);
int8_t compiler_do_sum(Compiler *self, Lexer *lexer, const charspan *s, Program *pg);
int8_t compiler_do_equality(Compiler *self, Lexer *lexer, const charspan *s, Program *pg);
int8_t compiler_do_compare(Compiler *self, Lexer *lexer, const charspan *s, Program *pg);
int8_t compiler_do_and(Compiler *self, Lexer *lexer, const charspan *s, Program *pg);
int8_t compiler_do_or(Compiler *self, Lexer *lexer, const charspan *s, Program *pg);

int8_t compiler_do_vars(Compiler *self, Lexer *lexer, const charspan *s, Program *pg);
int8_t compiler_do_ifs(Compiler *self, Lexer *lexer, const charspan *s, Program *pg);
int8_t compiler_do_while(Compiler *self, Lexer *lexer, const charspan *s, Program *pg);
int8_t compiler_do_for(Compiler *self, Lexer *lexer, const charspan *s, Program *pg);
int8_t compiler_do_break(Compiler *self, Lexer *lexer, const charspan *s, Program *pg);
int8_t compiler_do_continue(Compiler *self, Lexer *lexer, const charspan *s, Program *pg);
int8_t compiler_do_ret(Compiler *self, Lexer *lexer, const charspan *s, Program *pg);
int8_t compiler_do_throw(Compiler *self, Lexer *lexer, const charspan *s, Program *pg);
int8_t compiler_do_try_catch(Compiler *self, Lexer *lexer, const charspan *s, Program *pg);
int8_t compiler_do_expr_stmt(Compiler *self, Lexer *lexer, const charspan *s, Program *pg);
int8_t compiler_do_func(Compiler *self, Lexer *lexer, const charspan *s, Program *pg);
int8_t compiler_do_nestable_stmt(Compiler *self, Lexer *lexer, const charspan *s, Program *pg);
int8_t compiler_do_block(Compiler *self, Lexer *lexer, const charspan *s, Program *pg);
int8_t compiler_do_stmt(Compiler *self, Lexer *lexer, const charspan *s, Program *pg);

int8_t compiler_do_source(Compiler *self, Lexer *lexer, const charspan *s, Program *pg);

#endif
