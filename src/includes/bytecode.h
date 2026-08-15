#ifndef TBASIC_BYTECODE_H
#define TBASIC_BYTECODE_H

#include <stdint.h>
#include "mystr.h"
#include "value.h"
#include "vec.h"



#define TBASIC_PG_MARK_MODULE -1
#define TBASIC_PG_MARK_INVALID -2
#define TBASIC_RET_MARK_LAST 15

typedef struct code_vec_t Instruction;

void Instruction_dud(Instruction* ins);
void Instruction_new(MAYBE_UNUSED Instruction *ins);
void Instruction_copy(MAYBE_UNUSED Instruction *ins, MAYBE_UNUSED const Instruction *other);
void Instruction_move(MAYBE_UNUSED Instruction *ins, MAYBE_UNUSED Instruction *other);
void Instruction_del(MAYBE_UNUSED Instruction *ins);

typedef struct code_chunk_t Chunk;

void Chunk_dud(Chunk* c);
void Chunk_new(Chunk *c);
void Chunk_copy(Chunk *c, const Chunk *other);
void Chunk_move(Chunk *c, Chunk *other);
void Chunk_del(Chunk *c);

const Instruction *Chunk_code(const Chunk *c);
const Value *Chunk_constants(const Chunk *c);

STUB_VEC(Instruction)

STUB_VEC(Chunk)

STUB_VEC(mystr)

typedef enum vm_opcode_t : uint8_t {
    op_nop,
    op_put_none,
    op_put_bool,
    op_reserve,         // ? loads N NIL values when a function starts, reserving space for hoisted variables
    op_load_imm_gid,    // ? loads an immediate procedure ID --> chunk ID to dispatch to.
    op_load_local,
    op_store_local,
    op_bind_lstmp,      // ? Args: <local-ID>, takes a referenced list and index int, binding that indexed temporary to a local by ID.
    op_put_k,
    op_dup,
    op_pop,
    op_load_string_k,
    op_load_err_ref,
    op_mk_list,
    op_mk_dict,
    op_mk_closure,    // ? ARGS: N; wraps a function's chunk in a closure, given its global ID remaining on the stack to consume. Takes N captures.
    op_get_idx,
    op_set_idx,
    op_chk_none,      // ? checks if a value is NIL
    op_mul,
    op_div,
    op_add,
    op_sub,
    op_eq,
    op_ne,
    op_lt,
    op_gt,
    op_jmp,
    op_jmp_false,
    op_jmp_if,
    op_call,
    op_native_call,
    op_put_callee,
    op_ret,
    op_try,
    op_raise_err,
    op_catch_err,
    op_abort_else,        // ? implements `ASSERT` behavior

    op_mul_kl,
    op_div_kl,
    op_add_kl,
    op_sub_kl,
    op_mul_kk,
    op_div_kk,
    op_add_kk,
    op_sub_kk,
    op_mul_ll,
    op_div_ll,
    op_add_ll,
    op_sub_ll,
} Opcode;

typedef struct code_vec_t {
    Opcode op;
    uint8_t flag;
    uint16_t wide;
} Instruction;

typedef struct code_chunk_dbg_info_t {
    charspan name;
    uint16_t line;
    uint16_t col;
} ChunkDbgInfo;

typedef struct code_chunk_t {
    AnyVec_Instruction code;
    AnyVec_Value constants;
    ChunkDbgInfo info;
} Chunk;

typedef struct vm_program_t {
    AnyVec_Chunk chunks;
    AnyVec_mystr strings;
    mystr source;
    int entry_id;
} Program;

void program_dud(Program *self);
void program_del(Program *self);
Program program_take(Program *self);
void dump_program(const Program *pg);

#endif
