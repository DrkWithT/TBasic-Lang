#ifndef TBASIC_OBJ_CLOSURE_H
#define TBASIC_OBJ_CLOSURE_H

#include "objects.h"
#include "bytecode.h"



typedef struct tb_closure_t {
    ObjBase base;
    const Chunk *bc;
    Value *upvals;
    uint32_t count;
    uint16_t cid;
} Closure;

Closure *alloc_closure(const Chunk *bc, const Value *sub_stack, uint32_t n, uint16_t chunk_id);

void closure_del_fn(void *self);

int8_t closure_as_bool_fn(const void *self);

Value closure_get_v_fn(const void *self, Value key);

int8_t closure_set_v_fn(void *self, Value key, Value item);

void closure_display_fn(const void *self, const void *vm);

uint8_t closure_invoke_fn(void *self, void *vm, Instruction *caller_ip, const Value *caller_cvp, Value *stack_p, int16_t argc);

#endif