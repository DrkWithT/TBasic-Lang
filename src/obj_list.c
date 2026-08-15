#include "basics.h"
#include "obj_list.h"

List *alloc_list(size_t initial_size) {
    const Value filler_v = make_value_none();
    List *temp = ALLOC_TYPE(List);

    temp->base = (ObjBase) {
        .meta = {
            .tag = otag_list,
            // TODO: use these flags for runtime mutability checks!
            .flags = oflag_mutable | oflag_iterable
        },
        // ? 2: Let's bind the scuffed vtable at runtime.
        .del = list_del_fn,
        .as_bool = list_as_bool_fn,
        .get_v = list_get_v_fn,
        .set_v = list_set_v_fn,
        .display = list_display_fn,
        .invoke = list_invoke
    };

    // ? 3: initialize a backing vector of Values to avoid accessing garbage data.
    AnyVec_Value_new(&temp->data, (initial_size > 0) ? initial_size : 1, &filler_v);

    return temp;
}

void list_del_fn(void *self) {
    List *list_self = (List *)self;

    AnyVec_Value_del(&list_self->data);
}

int8_t list_as_bool_fn(const void *self) {
    const List *list_view = (const List *)self;

    return AnyVec_Value_len(&list_view->data) > 0;
}

Value list_get_v_fn(const void *self, Value key) {
    const List *list_self = (const List *)self;
    const int index = (key.tag == vtag_int) ? key.data.i : -1;
    const int index_end = AnyVec_Value_len(&list_self->data);

    if (index < 0 || index >= index_end) {
        return make_value_none();
    }

    return *AnyVec_Value_get(&list_self->data, index);
}

int8_t list_set_v_fn(void *self, Value key, Value item) {
    List *list_self = (List *)self;
    const int index = (key.tag == vtag_int) ? key.data.i : -1;
    const int index_end = AnyVec_Value_len(&list_self->data);

    if (key.tag == vtag_nil) {
        // ? NOTE: list[nil] is an implicit push. This is the idiom to expand a list.
        AnyVec_Value_push(&list_self->data, &item);
    } else if (index >= 0 && index < index_end) {
        *AnyVec_Value_getm(&list_self->data, index) = item;
    } else {
        return 0;
    }

    return 1;
}

void list_display_fn(const void *self, const void *vm) {
    const List *list = (const List *)self;
    const size_t end_pos = AnyVec_Value_len(&list->data);

    for (size_t i = 0; i < end_pos; i++) {
        const Value *temp = AnyVec_Value_get(&list->data, i);

        if (temp != NULL) {
            print_value(temp, vm);
            fprintf(stdout, " ");
        }
    }

    fprintf(stdout, "\n");
}

uint8_t list_invoke(void *self, void *vm, Instruction *caller_ip, const Value *caller_cvp, Value *stack_p, int16_t argc) {
    return 0;
}

size_t list_len(const void *self) {
    const List *list = (const List *)self;

    return AnyVec_Value_len(&list->data);
}
