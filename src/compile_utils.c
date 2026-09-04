#include "compile_utils.h"



IMPL_SCALAR_VEC(int)

IMPL_SCALAR_VEC(SymbolNote)

SymbolInfo make_symbol_info(charspan name_v, int16_t note_id, int16_t id, Domain d) {
    return (SymbolInfo) {
        .name = name_v,
        .api_note_id = note_id,
        .id = id,
        .domain = d
    };
}

SymbolTable make_symbol_table() {
    SymbolInfo *temp_infos = calloc(DEFAULT_SYMBOL_COUNT, sizeof(SymbolInfo));
    ScalarVec_SymbolNote temp_notes;
    ScalarVec_SymbolNote_new(&temp_notes, DEFAULT_SYMBOL_COUNT, (SymbolNote) {
        .msg = {
            .data = NULL,
            .length = 0
        },
        .tag = tb_api_none
    });

    if (temp_infos != NULL) {   
        return (SymbolTable) {
            .notes = temp_notes,
            .infos = temp_infos,
            .length = 0,
            .capacity = DEFAULT_SYMBOL_COUNT,
            .var_alloc_ip = 0,
            .local_argc = 0,
            .next_local_id = 0,     // ? Start from BP since BP holds the callee... OLD + 1 --> new ID, but top-level local names are globals which begin from 0.
        };
    }

    return (SymbolTable) {
        .notes = {
            .data = NULL,
            .capacity = 0,
            .length = 0
        },
        .infos = NULL,
        .length = 0,
        .capacity = 0,
        .var_alloc_ip = 0,
        .local_argc = 0,
        .next_local_id = 0
    };
}

void SymbolTable_dud(SymbolTable *self) {
    SymbolInfo *temp_infos = calloc(DEFAULT_SYMBOL_COUNT, sizeof(SymbolInfo));

    if (temp_infos != NULL) {
        ScalarVec_SymbolNote temp_notes;
        ScalarVec_SymbolNote_new(&temp_notes, DEFAULT_SYMBOL_COUNT, (SymbolNote) {
            .msg = { .data = NULL, .length = 0 },
            .tag = tb_api_none
        });

        self->notes = temp_notes;
        self->infos = temp_infos;
        self->length = 0;
        self->capacity = DEFAULT_SYMBOL_COUNT;

        for (int i = 0; i < self->capacity; i++) {
            self->infos[i] = (SymbolInfo) {
                .name = {
                    .data = NULL,
                    .length = 0
                },
                .id = 0,
                .domain = symbol_constant
            };
        }
    } else {
        self->notes = (ScalarVec_SymbolNote) {
            .data = NULL,
            .capacity = 0,
            .length = 0
        };
        self->infos = NULL;
        self->length = 0;
        self->capacity = 0;
    }

    self->var_alloc_ip = 0;
    self->local_argc = 0;
    self->next_local_id = 0;
}

void SymbolTable_del(SymbolTable *self) {
    ScalarVec_SymbolNote_del(&self->notes);

    if (self->infos != NULL) {
        free(self->infos);
        self->infos = NULL;
    }
}

void SymbolTable_copy(SymbolTable *dest, const SymbolTable *src) {
    if (dest == src) {
        return;
    }

    dest->notes = src->notes;
    dest->infos = src->infos;
    dest->length = src->length;
    dest->capacity = src->capacity;
    dest->var_alloc_ip = src->var_alloc_ip;
    dest->local_argc = src->local_argc;
    dest->next_local_id = src->next_local_id;
}

const SymbolInfo *SymbolTable_find(const SymbolTable *symbols, const charspan *s, Domain d) {
    const SymbolInfo *infos_begin = symbols->infos;
    const int entry_n = symbols->length;

    for (int entry_pos = 0; entry_pos < entry_n; entry_pos++) {
        if (charspan_equals_charspan(s, &infos_begin[entry_pos].name) && infos_begin[entry_pos].domain == d) {
            return infos_begin + entry_pos;
        }
    }

    return NULL;
}

const SymbolInfo *SymbolTable_push(SymbolTable *symbols, const SymbolInfo *info) {
    const int next_pos = symbols->length;
    const int old_capacity = symbols->capacity;

    if (next_pos >= old_capacity) {
        const int new_capacity = (old_capacity * 3) / 2;
        SymbolInfo *temp_data = realloc(symbols->infos, sizeof(SymbolInfo) * new_capacity);

        if (temp_data != NULL) {   
            symbols->infos = temp_data;
            symbols->capacity = new_capacity;
        } else {
            return NULL;
        }
    }

    symbols->infos[next_pos] = *info;
    symbols->length++;

    return symbols->infos + next_pos;
}

void annotate_symbol_table_at(SymbolTable *self, const charspan *symbol, SymbolNote note) {
    SymbolInfo *entries_it = self->infos;
    SymbolInfo *entries_end = self->infos + self->length;
    const int16_t next_note_id = self->notes.length;
    uint8_t info_found = 0;

    for (; entries_it != entries_end; entries_it++) {
        if (charspan_equals_charspan(&entries_it->name, symbol)) {
            entries_it->api_note_id = next_note_id;
            info_found = 1;
            break;
        }
    }

    if (!info_found) {
        return;
    }

    ScalarVec_SymbolNote_push(&self->notes, note);
}

IMPL_VEC(SymbolTable)



void ActiveLoop_dud(ActiveLoop *self) {
    ScalarVec_int_dud(&self->loop_breaks);
    ScalarVec_int_dud(&self->loop_continues);
}

void ActiveLoop_copy(ActiveLoop *self, const ActiveLoop *other) {
    ScalarVec_int_copy(&self->loop_breaks, &other->loop_breaks);
    ScalarVec_int_copy(&self->loop_continues, &other->loop_continues);
}

void ActiveLoop_del(ActiveLoop *self) {
    ScalarVec_int_del(&self->loop_breaks);
    ScalarVec_int_del(&self->loop_continues);
}



IMPL_VEC(ActiveLoop)
