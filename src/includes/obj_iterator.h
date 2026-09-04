#ifndef TBASIC_ITER_OBJ_H
#define TBASIC_ITER_OBJ_H



#include <stdio.h>
#include "objects.h"



// ? Helper function type for iteration, updating an iterator's inner cursor e.g `Value* p` for lists by advancing it. If the object is non-iterable (not an iterator), the function will return `false`.
typedef int8_t(*IterFn)(ObjMutPtr* self);

typedef enum tb_iter_tag_t : uint8_t {
    iter_t_list,
    iter_t_str,
} IterTag;

typedef struct tb_iter_t {
    ObjBase base;               // ? abstract metadata of object
    union {
        // void *opaque;        // ? maybe for dict nodes
        const Value *item_p;    // ? list item ptr
        const char *chr_p;      // ? string character ptr
    } data;
    const void* end_p;          // ? terminator ptr
    int16_t oid;                // ? Object-ID for GC tracking
    IterTag tag;                // ? error flags
} Iter;

Iter *alloc_iter_list(const ObjBase *obj, int16_t oid);
Iter *alloc_iter_str(const ObjBase *obj, int16_t oid);
void iter_del_fn(void *self);

int8_t iter_list_as_bool_fn(const void *self);
Value iter_list_get_v_fn(const void *self, Value key);
int8_t iter_list_set_v_fn(void *self, Value key, Value item);
void iter_list_display_fn(const void *self, const void *vm);

int8_t iter_str_as_bool_fn(const void *self);
Value iter_str_get_v_fn(const void *self, Value key);
int8_t iter_str_set_v_fn(void *self, Value key, Value item);
void iter_str_display_fn(const void *self, const void *vm);

// ? stub function: NOOP
uint8_t iter_invoke_fn(void *self, void *vm, const Instruction *caller_ip, const Value *caller_cvp, Value *stack_p, int16_t argc);

Value iter_list_iterate_fn(void* self, void *vm);
Value iter_str_iterate_fn(void* self, void *vm);

#endif