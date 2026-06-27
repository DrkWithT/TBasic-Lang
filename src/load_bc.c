#include <stdio.h>
#include "mystr.h"
#include "load_bc.h"



static int8_t tbl_can_run(const TBLoader *ld) {
    return ld->state != tbl_st_end && ld->state != tbl_err;
}

static int8_t tbl_eos(const TBLoader *ld) {
    return ld->pos >= ld->end;
}

static void tbl_set_flag(TBLoader* ld, TBLdErr err_flag) {
    ld->flags |= err_flag;
}

static uint8_t tbl_eat_u8(TBLoader *ld) {
    if (tbl_eos(ld)) {
        fprintf(stderr, "\x1b[1;31mLoad Err: failed to read byte at pos = %zu\x1b[0m\n", ld->pos);
        tbl_set_flag(ld, tbl_unexpected_eos);
        ld->state = tbl_err;

        return 0;
    }

    const uint8_t temp = ld->data[ld->pos];

    ld->pos++;

    return temp;
}

static int8_t tbl_chomp_n(TBLoader *ld, uint8_t n) {
    memset(&ld->chomp, 0, TBASIC_LOADER_CHOMP_SZ);

    const uint8_t clamped_rc = (n <= TBASIC_LOADER_CHOMP_SZ) ? n : TBASIC_LOADER_CHOMP_SZ ;

    uint8_t rc = 0;

    for (; rc < clamped_rc && !tbl_eos(ld); rc++, ld->pos++) {
        ld->chomp[rc] = ld->data[ld->pos];
    }

    return rc == n;
}

static int32_t tbl_eat_i32(TBLoader *ld) {
    const size_t i32_size = sizeof(int32_t);

    if (!tbl_chomp_n(ld, i32_size)) {
        fprintf(stderr, "\x1b[1;31mLoad Err: failed to read 4 bytes for i32 at pos = %zu\x1b[0m\n", ld->pos);
        tbl_set_flag(ld, tbl_unexpected_eos);
        ld->state = tbl_err;
        return 0;
    }

    int temp = 0;
    memcpy(&temp, ld->chomp, i32_size);

    return temp;
}

static uint16_t tbl_eat_u16(TBLoader *ld) {
    const size_t u16_size = sizeof(uint16_t);

    if (!tbl_chomp_n(ld, u16_size)) {
        fprintf(stderr, "\x1b[1;31mLoad Err: failed to read 2 bytes for u16 at pos = %zu\x1b[0m\n", ld->pos);
        tbl_set_flag(ld, tbl_unexpected_eos);
        ld->state = tbl_err;
        return 0;
    }

    uint16_t temp = 0;
    memcpy(&temp, ld->chomp, u16_size);

    return temp;
}

static uint32_t tbl_eat_u32(TBLoader *ld) {
    const size_t u32_size = sizeof(uint32_t);

    if (!tbl_chomp_n(ld, u32_size)) {
        fprintf(stderr, "\x1b[1;31mLoad Err: failed to read 4 bytes for u32 at pos = %zu\x1b[0m\n", ld->pos);
        tbl_set_flag(ld, tbl_unexpected_eos);
        ld->state = tbl_err;
        return 0;
    }

    uint32_t temp = 0;
    memcpy(&temp, ld->chomp, u32_size);

    return temp;
}

static size_t tbl_eat_u64(TBLoader *ld) {
    const size_t u64_size = sizeof(size_t);

    if (!tbl_chomp_n(ld, u64_size)) {
        fprintf(stderr, "\x1b[1;31mLoad Err: failed to read 4 bytes for u64 at pos = %zu\x1b[0m\n", ld->pos);
        tbl_set_flag(ld, tbl_unexpected_eos);
        ld->state = tbl_err;
        return 0;
    }

    size_t temp = 0;
    memcpy(&temp, ld->chomp, u64_size);

    return temp;
}

static void tbl_eat_prefix(TBLoader *ld) {
    // puts("tbl_eat_prefix");

    if (!tbl_chomp_n(ld, 4)) {
        fprintf(stderr, "\x1b[1;31mLoad Err: failed to read 4 bytes for prefix at pos = %zu\x1b[0m\n", ld->pos);
        tbl_set_flag(ld, tbl_invalid_byte);
        ld->state = tbl_err;
        return;
    }

    if (ld->chomp[0] == 'T' && ld->chomp[1] == 'B' && ld->chomp[2] == 'C' && ld->chomp[3] == 'F') {
        ld->state = tbl_st_eat_meta;
    } else {
        fprintf(stderr, "\x1b[1;31mLoad Err: no valid prefix at pos = %zu\x1b[0m\n", ld->pos);
        tbl_set_flag(ld, tbl_invalid_byte);
        ld->state = tbl_err;
    }
}

static void tbl_eat_meta(TBLoader *ld, Program *pg) {
    // puts("tbl_eat_meta");

    int entry_pos_i32 = tbl_eat_u32(ld);
    if (ld->state == tbl_err) {
        fprintf(stderr, "\x1b[1;31mLoad Err: failed to read 4 bytes for u32 at pos = %zu\x1b[0m\n", ld->pos);
        return;
    }

    pg->entry_id = entry_pos_i32;

    ld->state = tbl_st_eat_str;
}

static void tbl_eat_str(TBLoader *ld, Program *pg) {
    // puts("tbl_eat_str");

    const uint32_t ascii_len = tbl_eat_u32(ld);
    if (ld->state == tbl_err) {
        return;
    }

    // printf("Load Log: ascii_len = %d\n", ascii_len);

    if (ascii_len == 0) {
        ld->state = tbl_st_eat_chunk;
        return;
    }

    mystr temp;
    mystr_res(&temp, TBASIC_LOADER_DEFAULT_MYSTR_SZ);

    int32_t rc = ascii_len;
    for (; rc > 0; rc--) {
        const char c = tbl_eat_u8(ld);

        if (ld->state == tbl_err) {
            break;
        }

        if (!mystr_append_raw(&temp, &c, 1)) {
            break;
        }
    }

    if (rc != 0) {
        return;
    }

    AnyVec_mystr_push(&pg->strings, &temp);
    ld->state = tbl_st_eat_str;
}

static int32_t alias_chomp_as_i32(const uint8_t *data) {
    int32_t temp_i32 = 0;

    memcpy(&temp_i32, data, sizeof(int32_t));
    return temp_i32;
}

static float alias_chomp_as_f32(const uint8_t *data) {
    float temp_f32 = 0;

    memcpy(&temp_f32, data, sizeof(float));
    return temp_f32;
}

static void tbl_eat_chunk(TBLoader *ld, Program *pg) {
    // puts("tbl_eat_chunk");

    const size_t bc_len = tbl_eat_u64(ld);
    if (ld->state == tbl_err) {
        fprintf(stderr, "\x1b[1;31mLoad Err: failed to read chunk-length at pos = %zu\x1b[0m\n", ld->pos);
        return;
    }

    // printf("Load Log: bc_len = %zu\n", bc_len);

    if (bc_len == 0) {
        // puts("Load Log: There are no more chunks to read.");
        ld->state = tbl_st_end;
        return;
    }

    AnyVec_Instruction temp_bc;
    AnyVec_Instruction_dud(&temp_bc);

    size_t bc_rc = bc_len;
    for (; bc_rc > 0; bc_rc--) {
        const Opcode temp_op = tbl_eat_u8(ld);
        if (ld->state == tbl_err) {
            fprintf(stderr, "\x1b[1;31mLoad Err: failed to read opcode at pos = %zu\x1b[0m\n", ld->pos);
            break;
        }

        const uint8_t temp_flags = tbl_eat_u8(ld);
        if (ld->state == tbl_err) {
            fprintf(stderr, "\x1b[1;31mLoad Err: failed to read opcode flags at pos = %zu\x1b[0m\n", ld->pos);
            break;
        }

        const uint32_t temp_arg = tbl_eat_u16(ld);
        if (ld->state == tbl_err) {
            fprintf(stderr, "\x1b[1;31mLoad Err: failed to read opcode arg at pos = %zu\x1b[0m\n", ld->pos);
            break;
        }

        const Instruction temp_inst = {
            .op = temp_op,
            .flag = temp_flags,
            .wide = temp_arg
        };

        AnyVec_Instruction_push(&temp_bc, &temp_inst);
    }

    if (bc_rc != 0) {
        fprintf(stderr, "\x1b[1;31mLoad Err: failed to all bytecode, now around pos = %zu\x1b[0m\n", ld->pos);
        AnyVec_Instruction_del(&temp_bc);
        ld->state = tbl_err;
        return;
    }



    const size_t kv_len = tbl_eat_u64(ld);
    if (ld->state == tbl_err) {
        fprintf(stderr, "\x1b[1;31mLoad Err: failed to read chunk constant count at pos = %zu\x1b[0m\n", ld->pos);
        return;
    }

    AnyVec_Value temp_kv;
    AnyVec_Value_dud(&temp_kv);

    size_t kv_rc = kv_len;
    for (; kv_rc > 0; kv_rc--) {
        const ValTag temp_tag = tbl_eat_u8(ld);
        if (ld->state == tbl_err) {
            fprintf(stderr, "\x1b[1;31mLoad Err: failed to read constant value tag at pos = %zu\x1b[0m\n", ld->pos);
            break;
        }

        // ! Largest serialized blob possible for a Value is a float bit-pattern: 4 bytes on x64, for example.
        if (!tbl_chomp_n(ld, sizeof(float))) {
            fprintf(stderr, "\x1b[1;31mLoad Err: failed to read serialized value union at pos = %zu\x1b[0m\n", ld->pos);
            break;
        }

        Value temp_konst;

        switch (temp_tag) {
            case vtag_nil:
                temp_konst = make_value_none();
                break;
            case vtag_bool:
                temp_konst = make_value_bool(ld->chomp[0] != 0);
                break;
            case vtag_int:
                temp_konst = make_value_int(alias_chomp_as_i32(ld->chomp));
                break;
            case vtag_real:
                temp_konst = make_value_int(alias_chomp_as_f32(ld->chomp));
                break; 
            case vtag_strid:
                temp_konst = make_value_str(alias_chomp_as_i32(ld->chomp));
                break; 
            default:
                temp_konst = make_value_none();
                break;
        }

        AnyVec_Value_push(&temp_kv, &temp_konst);
    }

    if (kv_rc != 0) {
        fprintf(stderr, "\x1b[1;31mLoad Err: failed to all bytecode, now around pos = %zu\x1b[0m\n", ld->pos);
        AnyVec_Instruction_del(&temp_bc);
        AnyVec_Value_del(&temp_kv);
        ld->state = tbl_err;
        return;
    }

    const Chunk temp_chunk = {
        .code = temp_bc,
        .constants = temp_kv
    };

    AnyVec_Chunk_push(&pg->chunks, &temp_chunk);
    ld->state = tbl_st_eat_chunk;
}

static void tbl_dispatch(TBLoader *ld, Program *pg) {
    while (tbl_can_run(ld)) {
        switch (ld->state) {
            case tbl_st_eat_prefix: tbl_eat_prefix(ld); break;
            case tbl_st_eat_meta: tbl_eat_meta(ld, pg); break;
            case tbl_st_eat_str: tbl_eat_str(ld, pg); break;
            case tbl_st_eat_chunk: tbl_eat_chunk(ld, pg); break;
            default: goto tbl_end_dispatch;
        }
    }

tbl_end_dispatch:
    return;
}



void tbl_dud(TBLoader *ld) {
    ld->data = NULL;
    ld->pos = 0;
    ld->end = 0;

    memset(&ld->chomp, 0, TBASIC_LOADER_CHOMP_SZ);

    ld->flags = 0x00;
    ld->state = tbl_st_eat_prefix;
}

Program tbl_load_from(TBLoader *ld, const uint8_t *data, size_t n) {
    ld->data = data;
    ld->pos = 0;
    ld->end = n;

    Program prgm;
    program_dud(&prgm);

    tbl_dispatch(ld, &prgm);

    if (ld->state == tbl_err) {
        program_del(&prgm);
        prgm = (Program) {
            .chunks = {
                .data = NULL,
                .capacity = 0,
                .length = 0,
            },
            .strings = {
                .data = NULL,
                .capacity = 0,
                .length = 0,
            },
            .entry_id = 0
        };
    }

    if (0 != (ld->flags & tbl_unexpected_eos)) {
        puts("\x1b[1;31mLoad Error: unexpected EOS in data!\x1b[0m");
    } else if (0 != (ld->flags & tbl_invalid_byte)) {
        printf("\x1b[1;31mLoad Error: unexpected byte found in data, it is malformed.\x1b[0m");
    }

    return prgm;
}
