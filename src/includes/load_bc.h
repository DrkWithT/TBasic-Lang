#ifndef TBASIC_LOAD_BC_H
#define TBASIC_LOAD_BC_H

#include <string.h>

#include "bytecode.h"



#define TBASIC_LOADER_CHOMP_SZ 8
#define TBASIC_LOADER_ZERO_V 0
#define TBASIC_LOADER_DEFAULT_MYSTR_SZ 8

typedef enum tbasic_loader_state_t : uint8_t {
    tbl_st_eat_prefix, // ? Eat & check magic prefix

    tbl_st_eat_meta,   // ? Eat all metadata fields

    tbl_st_eat_str,

    tbl_st_eat_chunk, // ? Eat chunk: exit early if `code_len == 0`

    tbl_st_end,
    tbl_err,
} TBLdState;

typedef enum tbasic_meta_tag_t : uint8_t {
    tbl_mt_entry_pos,
} TBLdMetaTag;

typedef enum tbasic_loader_err_t : uint8_t {
    tbl_invalid_byte = 0b00000001,
    tbl_unexpected_eos = 0b00000010,
} TBLdErr;

typedef struct tbasic_loader_t {
    const uint8_t *data;
    size_t pos;
    size_t end;
    uint8_t chomp[TBASIC_LOADER_CHOMP_SZ];
    uint8_t flags;
    TBLdState state;
} TBLoader;

void tbl_dud(TBLoader *ld);

Program tbl_load_from(TBLoader *ld, const uint8_t *data, size_t n);

#endif