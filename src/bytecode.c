#include "bytecode.h"

static const char *opcode_names[] = {
    "nop",
    "op_put_none",
    "op_put_bool",
    "op_load_imm_gid",
    "op_load_local",
    "op_store_local",
    "op_put_konst",
    "op_dup",
    "op_pop",
    "op_load_string_k",
    "op_mk_list",
    "op_mk_dict",
    "op_get_idx",
    "op_set_idx",
    "op_chk_none",
    "op_mul",
    "op_div",
    "op_add",
    "op_sub",
    "op_eq",
    "op_ne",
    "op_lt",
    "op_gt",
    "op_jmp",
    "op_jmp_false",
    "op_jmp_if",
    "op_call",
    "op_put_callee",
    "op_ret"
};

void Instruction_dud(Instruction* ins) {}
void Instruction_new(MAYBE_UNUSED Instruction *ins) {}
void Instruction_copy(MAYBE_UNUSED Instruction *ins, MAYBE_UNUSED const Instruction *other) {}
void Instruction_move(MAYBE_UNUSED Instruction *ins, MAYBE_UNUSED Instruction *other) {}
void Instruction_del(MAYBE_UNUSED Instruction *ins) {}

IMPL_VEC(Instruction)



void Chunk_dud(Chunk* c) {
    AnyVec_Instruction_dud(&c->code);
    AnyVec_Value_dud(&c->constants);
}

void Chunk_new(Chunk *c) {
    AnyVec_Instruction_dud(&c->code);
    AnyVec_Value_dud(&c->constants);
}

void Chunk_copy(Chunk *c, const Chunk *other) {
    AnyVec_Instruction_copy(&c->code, &other->code);
    AnyVec_Value_copy(&c->constants, &other->constants);
}

void Chunk_move(Chunk *c, Chunk *other) {
    AnyVec_Instruction_move(&c->code, &other->code);
    AnyVec_Value_move(&c->constants, &other->constants);
}

void Chunk_del(MAYBE_UNUSED Chunk *c) {
    AnyVec_Instruction_del(&c->code);
    AnyVec_Value_del(&c->constants);
}

const Instruction *Chunk_code(const Chunk *c) {
    return c->code.data;
}

const Value *Chunk_constants(const Chunk *c) {
    return c->constants.data;
}

void dump_program(const Program *pg) {
    const size_t pg_chunks_n = AnyVec_Chunk_len(&pg->chunks);

    puts("---- BYTECODE DUMP ----\n");

    for (size_t chunk_pos = 0; chunk_pos < pg_chunks_n; chunk_pos++) {
        printf("CHUNK %zu:\n", chunk_pos);

        const Chunk *temp_chunk = AnyVec_Chunk_get(&pg->chunks, chunk_pos);
        const size_t temp_chunk_const_n = AnyVec_Value_len(&temp_chunk->constants);
        const size_t temp_chunk_code_n = AnyVec_Instruction_len(&temp_chunk->code);

        puts("CONSTANTS:\n");

        for (size_t temp_chunk_const_id = 0; temp_chunk_const_id < temp_chunk_const_n; temp_chunk_const_id++) {
            printf("\tconst%zu = ", temp_chunk_const_id);
            print_value(AnyVec_Value_get(&temp_chunk->constants, temp_chunk_const_id), NULL);
            printf("\n");
        }

        puts("\nCODE:\n");

        for (size_t temp_chunk_code_id = 0; temp_chunk_code_id < temp_chunk_code_n; temp_chunk_code_id++) {
            const Instruction *ins = AnyVec_Instruction_get(&temp_chunk->code, temp_chunk_code_id);
            printf("\t%zu: %s   %d, %d\n", temp_chunk_code_id, opcode_names[ins->op], ins->flag, ins->wide);
        }

        printf("\n");
    }
}

IMPL_VEC(Chunk)

IMPL_VEC(mystr)

void program_dud(Program *self) {
    AnyVec_Chunk_dud(&self->chunks);
    AnyVec_mystr_dud(&self->strings);
    self->entry_id = 0;
}

void program_del(Program *self) {
    AnyVec_Chunk_del(&self->chunks);
    AnyVec_mystr_del(&self->strings);
    self->entry_id = -1;
}
