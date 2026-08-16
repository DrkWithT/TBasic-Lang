#include "bytecode.h"

static const char *opcode_names[] = {
    "nop",
    "op_put_none",
    "op_put_bool",
    "op_load_imm_gid",
    "op_load_local",
    "op_store_local",
    "op_bind_lstmp",
    "op_put_konst",
    "op_dup",
    "op_pop",
    "op_load_string_k",
    "op_load_err_ref",
    "op_mk_list",
    "op_mk_dict",
    "op_mk_closure",
    "op_get_idx",
    "op_set_idx",
    "op_get_upv",
    "op_set_upv",
    "op_chk_none",
    "op_bit_not",
    "op_mul",
    "op_div",
    "op_add",
    "op_sub",
    "op_bit_and",
    "op_bit_or",
    "op_bit_xor",
    "op_bit_shl",
    "op_bit_shr",
    "op_eq",
    "op_ne",
    "op_lt",
    "op_gt",
    "op_jmp",
    "op_jmp_false",
    "op_jmp_if",
    "op_call",
    "op_native_call",
    "op_put_callee",
    "op_ret",
    "op_try",
    "op_raise_err",
    "op_catch_err",
    "op_abort_if",
    "op_mul_kl",
    "op_div_kl",
    "op_add_kl",
    "op_sub_kl",
    "op_mul_kk",
    "op_div_kk",
    "op_add_kk",
    "op_sub_kk",
    "op_mul_ll",
    "op_div_ll",
    "op_add_ll",
    "op_sub_ll",
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
    c->info = (ChunkDbgInfo) {
        .name = (charspan) {
            .data = "??",
            .length = 2
        },
        .line = 0,
        .col = 0
    };
    c->local_slots = 0;
}

void Chunk_new(Chunk *c) {
    AnyVec_Instruction_dud(&c->code);
    AnyVec_Value_dud(&c->constants);
    c->info = (ChunkDbgInfo) {
        .name = (charspan) {
            .data = "??",
            .length = 2
        },
        .line = 0,
        .col = 0
    };
    c->local_slots = 0;
}

void Chunk_copy(Chunk *c, const Chunk *other) {
    AnyVec_Instruction_copy(&c->code, &other->code);
    AnyVec_Value_copy(&c->constants, &other->constants);
    c->info = other->info;
}

void Chunk_move(Chunk *c, Chunk *other) {
    AnyVec_Instruction_move(&c->code, &other->code);
    AnyVec_Value_move(&c->constants, &other->constants);
    c->info = other->info;
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

        printf("\tRESERVED: %d locals\n", temp_chunk->local_slots);
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
    self->source = (mystr) {.data = NULL, .capacity = 0, .length = 0},
    self->entry_id = 0;
}

void program_del(Program *self) {
    AnyVec_Chunk_del(&self->chunks);
    AnyVec_mystr_del(&self->strings);
    mystr_del(&self->source);
    self->entry_id = TBASIC_PG_MARK_INVALID;
}

Program program_take(Program *self) {
    Program temp = *self;

    self->chunks.data = NULL;
    self->strings.data = NULL;
    self->source.data = NULL;
    self->entry_id = TBASIC_PG_MARK_INVALID;

    return temp;
}
