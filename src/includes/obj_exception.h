#ifndef TBASIC_OBJ_EXCEPTION_H
#define TBASIC_OBJ_EXCEPTION_H

#include "value.h"
#include "objects.h"



// ? Represents a lightweight exception type with message and error. Its semantics are a pair-like object of `["msg", line_no]`.
typedef struct tb_err_t {
    ObjBase base;
    Value data;
    uint16_t line;
} TBErr;

TBErr *alloc_tberr(const Value *data_v, uint16_t line);

void tberr_del_fn(void *self);
int8_t tberr_as_bool_fn(const void *self);
Value tberr_get_v_fn(const void *self, Value key);
int8_t tberr_set_v_fn(void *self, Value key, Value item);
void tberr_display_fn(const void *self, const void *vm);

#endif