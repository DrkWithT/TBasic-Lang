#ifndef TBASIC_OBJ_FILESTREAM_H
#define TBASIC_OBJ_FILESTREAM_H

#include <stdio.h>
#include "objects.h"



typedef struct tb_fs_t {
    ObjBase base;
    FILE *f;
    const char *name;
} FileStream;

FileStream *alloc_fs(const char *path, int mode);

void fs_del_fn(void *self);
int8_t fs_as_bool_fn(const void *self);
Value fs_get_v_fn(const void *self, Value key);
int8_t fs_set_v_fn(void *self, Value key, Value item);
void fs_display_fn(const void *self, const void *vm_state);

// ? stub function: NOOP
uint8_t fs_invoke(void *self, void *vm, const Instruction *caller_ip, const Value *caller_cvp, Value *stack_p, int16_t argc);

Value fs_iterate_fn(void* self, void *vm);

#endif