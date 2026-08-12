#ifndef TBASIC_LIST_OBJECT_H
#define TBASIC_LIST_OBJECT_H

#include "value.h"
#include "objects.h"



typedef struct ts_list_obj_t {
    ObjBase base;       // ? stores discriminator, metadata flags, and vtable
    AnyVec_Value data;
} List;

List *alloc_list(size_t initial_size);

void list_del_fn(void *self);
int8_t list_as_bool_fn(const void *self);
Value list_get_v_fn(const void *self, Value key);
int8_t list_set_v_fn(void *self, Value key, Value item);
void list_display_fn(const void *self, const void *vm);

// ! EXTRA method for querying object length / size. See README.
size_t list_len(const void *self);

#endif
