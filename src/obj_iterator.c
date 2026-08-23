#include "obj_iterator.h"
#include "obj_list.h"
#include "obj_str.h"



Iter *alloc_iter_list(const ObjBase *obj, int16_t oid) {
    Iter *temp = ALLOC_TYPE(Iter);

    temp->base = (ObjBase) {
        .meta = {
            .tag = otag_iter,
            .flags = oflag_iterable
        },
        .del = iter_del_fn,
        .as_bool = iter_list_as_bool_fn,
        .get_v = iter_list_get_v_fn,
        .set_v = iter_list_set_v_fn,
        .display = iter_list_display_fn,
        .invoke = iter_invoke_fn,
        .iterate = iter_list_iterate_fn
    };

    const List *temp_ls = (const List *)obj;

    temp->data.item_p = temp_ls->data.data;
    temp->end_p = temp_ls->data.data + temp_ls->data.length;
    temp->oid = oid;
    temp->tag = iter_t_list;

    return temp;
}

Iter *alloc_iter_str(const ObjBase *obj, int16_t oid) {
    Iter *temp = ALLOC_TYPE(Iter);

    temp->base = (ObjBase) {
        .meta = {
            .tag = otag_iter,
            .flags = oflag_iterable
        },
        .del = iter_del_fn,
        .as_bool = iter_str_as_bool_fn,
        .get_v = iter_str_get_v_fn,
        .set_v = iter_str_set_v_fn,
        .display = iter_str_display_fn,
        .invoke = iter_invoke_fn,
        .iterate = iter_str_iterate_fn
    };

    const String *temp_ls = (const String *)obj;

    temp->data.chr_p = temp_ls->data.data;
    temp->end_p = temp_ls->data.data + temp_ls->data.length;
    temp->oid = oid;
    temp->tag = iter_t_str;

    return temp;
}

void iter_del_fn(void *self) {
    // ? This is NO-OP since iterators only view objects.
}

int8_t iter_list_as_bool_fn(const void *self) {
    const Iter *iter = (const Iter *)self;

    return iter->data.item_p != iter->end_p;
}

Value iter_list_get_v_fn(const void *self, Value key) {
    const Iter *iter = (const Iter *)self;

    if (iter->data.item_p == iter->end_p) {
        return make_value_none();
    }

    return *iter->data.item_p;
}

int8_t iter_list_set_v_fn(void *self, Value key, Value item) {
    return 0; // ? NO-OP
}

void iter_list_display_fn(const void *self, const void *vm) {
    const Iter *iter = (const Iter *)self;

    printf("Iter(tag = 'list', cursor = %p, end = %p)", iter->data.item_p, iter->end_p);
}

int8_t iter_str_as_bool_fn(const void *self) {
    const Iter *iter = (const Iter *)self;

    return iter->data.chr_p != iter->end_p;
}

Value iter_str_get_v_fn(const void *self, Value key) {
    const Iter *iter = (const Iter *)self;

    if (iter->data.chr_p == iter->end_p) {
        return make_value_none();
    }

    return make_value_int(*iter->data.chr_p);
}

int8_t iter_str_set_v_fn(void *self, Value key, Value item) {
    return 0; // ? NO-OP
}

void iter_str_display_fn(const void *self, const void *vm) {
    const Iter *iter = (const Iter *)self;

    printf("Iter(tag = 'str', cursor = %p, end = %p)", iter->data.chr_p, iter->end_p);
}

uint8_t iter_invoke_fn(void *self, void *vm, const Instruction *caller_ip, const Value *caller_cvp, Value *stack_p, int16_t argc) {
    return 0; // ? stub function: NOOP
}

Value iter_list_iterate_fn(void* self, void *vm) {
    Iter *iter = (Iter *)self;

    if (iter->data.item_p == iter->end_p) {
        return make_value_bool(0);
    }

    iter->data.item_p++;

    return make_value_bool(1);
}

Value iter_str_iterate_fn(void* self, void *vm) {
    Iter *iter = (Iter *)self;

    if (iter->data.chr_p == iter->end_p) {
        return make_value_bool(0);
    }

    iter->data.chr_p++;

    return make_value_bool(1);
}
