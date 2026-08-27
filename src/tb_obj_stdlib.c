#include "obj_iterator.h"
#include "tb_obj_stdlib.h"



VMStatus native_mkiter(VMState *s) {
    const Value arg = s->stack[s->bp + 1];

    if (arg.tag != vtag_obj_id) {
        s->sp++;
        s->stack[s->sp] = make_value_none();

        return vm_status_pending;
    }

    ObjPtr temp_obj = heap_get(&s->heap, arg.data.obj_id);

    if (!temp_obj) {
        s->sp++;
        s->stack[s->sp] = make_value_none();

        return vm_status_pending;
    }

    Iter *temp_it = NULL;

    if (temp_obj->meta.tag == otag_list) {
        temp_it = alloc_iter_list(temp_obj, arg.data.obj_id);
    } else if (temp_obj->meta.tag == otag_string) {
        temp_it = alloc_iter_str(temp_obj, arg.data.obj_id);
    }

    if (!temp_it) {
        s->sp++;
        s->stack[s->sp] = make_value_none();

        return vm_status_pending;
    }

    s->sp++;
    s->stack[s->sp] = make_value_obj(heap_store(&s->heap, (ObjMutPtr)temp_it));

    return vm_status_pending;
}

VMStatus native_mviter(VMState *s) {
    const Value arg = s->stack[s->bp + 1];

    if (arg.tag != vtag_obj_id) {
        s->sp++;
        s->stack[s->sp] = make_value_none();

        return vm_status_pending;
    }

    ObjMutPtr temp_obj = heap_getm(&s->heap, arg.data.obj_id);

    if (!temp_obj) {
        s->sp++;
        s->stack[s->sp] = make_value_none();

        return vm_status_pending;
    }

    s->sp++;
    s->stack[s->sp] = temp_obj->iterate(temp_obj, s);

    return vm_status_pending;
}

VMStatus native_pkiter(VMState *s) {
    const Value arg = s->stack[s->bp + 1];

    if (arg.tag != vtag_obj_id) {
        s->sp++;
        s->stack[s->sp] = make_value_none();

        return vm_status_pending;
    }

    ObjMutPtr temp_obj = heap_getm(&s->heap, arg.data.obj_id);

    if (!temp_obj) {
        s->sp++;
        s->stack[s->sp] = make_value_none();

        return vm_status_pending;
    }

    // ? ITERATOR.GET_V() will unconditionally get the cursor's Value if the iterator isn't exhausted. A NIL key is not a problem.
    s->sp++;
    s->stack[s->sp] = temp_obj->get_v(temp_obj, make_value_none());

    return vm_status_pending;
}

VMStatus native_thaw(VMState *s) {
    const Value arg = s->stack[s->bp + 1];

    if (arg.tag != vtag_obj_id) {
        s->sp++;
        s->stack[s->sp] = make_value_bool(0);

        return vm_status_pending;
    }

    ObjMutPtr temp_obj = heap_getm(&s->heap, arg.data.obj_id);
    const int8_t obj_valid = temp_obj != NULL;

    if (obj_valid) {
        object_base_flag_on(temp_obj, oflag_mutable);
    }

    s->sp++;
    s->stack[s->sp] = make_value_bool(obj_valid);
    
    return vm_status_pending;
}

VMStatus native_freeze(VMState *s) {
    const Value arg = s->stack[s->bp + 1];

    if (arg.tag != vtag_obj_id) {
        s->sp++;
        s->stack[s->sp] = make_value_bool(0);

        return vm_status_pending;
    }

    ObjMutPtr temp_obj = heap_getm(&s->heap, arg.data.obj_id);
    const int8_t obj_valid = temp_obj != NULL;

    if (obj_valid) {
        object_base_flag_off(temp_obj, oflag_mutable);
    }

    s->sp++;
    s->stack[s->sp] = make_value_bool(obj_valid);

    return vm_status_pending;
}
