#ifndef TBASIC_COMPILER_H
#define TBASIC_COMPILER_H

#include "mystr.h"
#include "lex.h"
#include "bytecode.h"
#include "compile_utils.h"

#define TBASIC_MAX_COMPILE_ERRORS 5



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

uint8_t compiler_do_list(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints);
uint8_t compiler_do_dict(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints);
uint8_t compiler_do_literal(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints);
uint8_t compiler_do_lhs(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints);
uint8_t compiler_do_call(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints);
uint8_t compiler_do_unary(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints);
uint8_t compiler_do_null_coal(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints);
uint8_t compiler_do_factor(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints);
uint8_t compiler_do_sum(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints);
uint8_t compiler_do_equality(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints);
uint8_t compiler_do_compare(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints);
uint8_t compiler_do_and(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints);
uint8_t compiler_do_or(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints);

uint8_t compiler_do_vars(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints);
uint8_t compiler_do_ifs(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints);
uint8_t compiler_do_while(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints);
uint8_t compiler_do_for(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints);
uint8_t compiler_do_break(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints);
uint8_t compiler_do_continue(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints);
uint8_t compiler_do_ret(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints);
uint8_t compiler_do_throw(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints);
uint8_t compiler_do_try_catch(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints);
uint8_t compiler_do_expr_stmt(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints);
uint8_t compiler_do_func(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints);
uint8_t compiler_do_nestable_stmt(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints);
uint8_t compiler_do_block(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints);
uint8_t compiler_do_stmt(Compiler *self, Lexer *lexer, const charspan *s, Program *pg, CompHints hints);

int8_t compiler_do_source(Compiler *self, Lexer *lexer, const charspan *s, Program *pg);

#endif
