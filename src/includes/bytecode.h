#ifndef TBASIC_BYTECODE_H
#define TBASIC_BYTECODE_H

#include <stdint.h>
#include "mystr.h"
#include "value.h"
#include "vec.h"



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
    op_put_k,
    op_dup,
    op_pop,
    op_load_string_k,
    op_mk_list,
    op_mk_dict,
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
    op_put_callee,
    op_ret
} Opcode;

typedef struct code_vec_t {
    Opcode op;
    uint8_t flag;
    uint16_t wide;
} Instruction;

typedef struct code_chunk_t {
    AnyVec_Instruction code;
    AnyVec_Value constants;
} Chunk;

typedef struct vm_program_t {
    AnyVec_Chunk chunks;
    AnyVec_mystr strings;
    int entry_id;
} Program;

void program_dud(Program *self);
void program_del(Program *self);
void dump_program(const Program *pg);

#endif
