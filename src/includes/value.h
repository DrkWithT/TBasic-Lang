#ifndef TBASIC_VALUE_H
#define TBASIC_VALUE_H

#include <stdint.h>
#include "basics.h"
#include "vec.h"



typedef enum vm_vtag_t : uint8_t {
    vtag_nil,
    vtag_bool,
    vtag_int,
    vtag_real,
    vtag_strid,         // ? Holds an ID into an interned string pool.
    vtag_obj_id,        // ? Holds an ID into an object pool.
} ValTag;

typedef struct vm_value_t {
    union {
        int8_t byte;
        int16_t obj_id;
        int i;          // ? This can be a 32-bit signed scalar / ID into a string pool, object pool
        float f;
    } data;

    ValTag tag;
} Value;

static inline Value make_value_none() {
    return (Value) {
        .data = {
            .byte = 0
        },
        .tag = vtag_nil
    };
}

static inline Value make_value_bool(int8_t b) {
    return (Value) {
        .data = {
            .byte = b
        },
        .tag = vtag_bool
    };
}

static inline Value make_value_obj(int16_t obj_id) {
    return (Value) {
        .data = {
            .obj_id = obj_id
        },
        .tag = vtag_obj_id
    };
}

static inline Value make_value_int(int i) {
    return (Value) {
        .data = {
            .i = i
        },
        .tag = vtag_int
    };
}

static inline Value make_value_real(float f) {
    return (Value) {
        .data = {
            .f = f
        },
        .tag = vtag_real
    };
}

static inline Value make_value_str(uint16_t str_id) {
    return (Value) {
        .data = {
            .i = str_id,
        },
        .tag = vtag_strid
    };
}

void Value_dud(Value* ins);
void Value_new(MAYBE_UNUSED Value *ins);
void Value_copy(MAYBE_UNUSED Value *ins, MAYBE_UNUSED const Value *other);
void Value_move(MAYBE_UNUSED Value *ins, MAYBE_UNUSED Value *other);
void Value_del(MAYBE_UNUSED Value *ins);

void print_string_k(const void *obj_ptr);
// ! FIXME: use void *ctx to VMState *s, then using its state to invoke stringification a heap object of ID = v.data.i ...
void print_object(const void *obj_ptr, const void *vm_state);
void print_value(const Value *v, const void *vm_state);

STUB_VEC(Value)



#endif