#include <stdlib.h>
#include "obj_exception.h"
#include "vm.h"



TBErr *alloc_tberr(const Value *data_v, uint16_t line) {
    TBErr *temp = ALLOC_TYPE(TBErr);

    if (temp != NULL) {
        temp->base = (ObjBase) {
            .meta = {
                .tag = otag_err,
                .flags = 0 // ? denotes immutability and non-iterability
            },
            .del = tberr_del_fn,
            .as_bool = tberr_as_bool_fn,
            .get_v = tberr_get_v_fn,
            .set_v = tberr_set_v_fn,
            .display = tberr_display_fn,
        };
        temp->data = *data_v;
        temp->line = line;
    }

    return temp;
}

void tberr_del_fn(void *self) {
    // ? exceptions are immutable
}

int8_t tberr_as_bool_fn(const void *self) {
    return 1; // ? exceptions are always truthy
}

Value tberr_get_fn(const void *self, Value key) {
    if (key.tag != vtag_int) {
        return make_value_none();
    }
    
    const TBErr *self_as_err = (const TBErr *)self;
    const int field_index = key.data.i;

    switch (field_index) {
        case 0: return self_as_err->data;
        case 1: return make_value_int(self_as_err->line);
        default: return make_value_none();
    }
}

int8_t tberr_set_fn(void *self, Value key, Value item) {
    return 1; // ? mutating an error object is a NOOP
}

void tberr_display_fn(const void *self, const void *vm) {
    const TBErr *self_as_err = (const TBErr *)self;

    if (self_as_err->base.meta.tag != otag_err) {
        return;
    }

    const VMState *vm_state_p = (const VMState *)vm;

    printf("Error at source ~ line %d:\n\tnote: ", self_as_err->line);
    print_value(&self_as_err->data, vm_state_p);
}
