#include "compile_utils.h"



IMPL_SCALAR_VEC(int)



SymbolInfo make_symbol_info(charspan name_v, int16_t id, Domain d) {
    return (SymbolInfo) {
        .name = name_v,
        .id = id,
        .domain = d
    };
}

SymbolTable make_symbol_table() {
    SymbolInfo *temp_infos = calloc(DEFAULT_SYMBOL_COUNT, sizeof(SymbolInfo));

    if (temp_infos != NULL) {   
        return (SymbolTable) {
            .infos = temp_infos,
            .length = 0,
            .capacity = DEFAULT_SYMBOL_COUNT,
            .local_argc = 0,
            .next_local_id = 0,     // ? Start from BP since BP holds the callee... OLD + 1 --> new ID.
        };
    }

    return (SymbolTable) {
        .infos = NULL,
        .length = 0,
        .capacity = 0,
        .local_argc = 0,
        .next_local_id = 0
    };
}

void SymbolTable_dud(SymbolTable *self) {
    SymbolInfo *temp_infos = calloc(DEFAULT_SYMBOL_COUNT, sizeof(SymbolInfo));

    if (temp_infos != NULL) {   
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
        self->infos = NULL;
        self->length = 0;
        self->capacity = 0;
    }

    self->local_argc = 0;
    self->next_local_id = 0;
}

void SymbolTable_del(SymbolTable *self) {
    if (self->infos != NULL) {
        free(self->infos);
        self->infos = NULL;
    }
}

void SymbolTable_copy(SymbolTable *dest, const SymbolTable *src) {
    if (dest == src) {
        return;
    }

    dest->infos = src->infos;
    dest->length = src->length;
    dest->capacity = src->capacity;
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
