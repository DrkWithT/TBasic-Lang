#include "obj_list.h"
#include "tb_list_stdlib.h"



VMStatus native_lsrev(VMState *s) {
    const Value arg = s->stack[s->bp + 1];

    if (arg.tag != vtag_obj_id) {
        s->sp++;
        s->stack[s->sp] = make_value_bool(0);

        return vm_status_pending;
    }

    ObjMutPtr dest_ptr = heap_getm(&s->heap, arg.data.obj_id);

    if (!dest_ptr || dest_ptr->meta.tag != otag_list) {
        s->sp++;
        s->stack[s->sp] = make_value_bool(0);

        return vm_status_pending;
    }

    List *dest_list = (List *)dest_ptr;
    Value* left = dest_list->data.data;
    Value* right = left + dest_list->data.length - 1;

    for (; left < right; left++, right--) {
        const Value temp = *left;
        *left = *right;
        *right = temp;
    }

    s->sp++;
    s->stack[s->sp] = make_value_bool(1);

    return vm_status_pending;
}

VMStatus native_lscat(VMState *s) {
    const Value arg1 = s->stack[s->bp + 1];
    const Value arg2 = s->stack[s->bp + 2];

    if (arg1.tag != vtag_obj_id || arg2.tag != vtag_obj_id) {
        s->sp++;
        s->stack[s->sp] = make_value_none();

        return vm_status_pending;
    }

    ObjMutPtr dest_ptr = heap_getm(&s->heap, arg1.data.obj_id);
    ObjPtr src_ptr = heap_get(&s->heap, arg2.data.obj_id);

    if (!dest_ptr || !src_ptr) {
        s->sp++;
        s->stack[s->sp] = make_value_none();

        return vm_status_pending;
    } else if (dest_ptr->meta.tag != otag_list || src_ptr->meta.tag != otag_list) {
        s->sp++;
        s->stack[s->sp] = make_value_none();

        return vm_status_pending;
    }

    const List *src_list = (const List *)src_ptr;
    size_t src_length = src_list->data.length;

    for (size_t copy_pos = 0; copy_pos < src_length; copy_pos++) {
        // ! Use built-in idiom of list[NIL] = x to push `x`.
        dest_ptr->set_v(dest_ptr, make_value_none(), src_list->data.data[copy_pos]);
    }

    // ! Here, return mutated LHS for convenience- what if the user wants to compose/chain these calls?
    s->sp++;
    s->stack[s->sp] = arg1;

    return vm_status_pending;
}

VMStatus native_lsclr(VMState *s) {
    const Value arg = s->stack[s->bp + 1];

    if (arg.tag != vtag_obj_id) {
        s->sp++;
        s->stack[s->sp] = make_value_bool(0);

        return vm_status_pending;
    }

    ObjMutPtr list_ref = heap_getm(&s->heap, arg.data.obj_id);
    const int8_t obj_is_valid = list_ref != NULL && list_ref->meta.tag == otag_list;

    if (obj_is_valid) {
        List *list_ptr = (List *)list_ref;
        const size_t list_length = list_ptr->data.length;

        // ! Here, lazy unset each item...
        for (size_t list_pos = 0; list_pos < list_length; list_pos++) {
            list_ptr->data.data[list_pos].tag = vtag_nil;
        }
    }

    s->sp++;
    s->stack[s->sp] = make_value_bool(obj_is_valid);

    return vm_status_pending;
}

VMStatus native_lscut(VMState *s) {
    const Value arg = s->stack[s->bp + 1];
    const Value arg_begin = s->stack[s->bp + 2];
    const Value arg_len = s->stack[s->bp + 3];

    if (arg.tag != vtag_obj_id || arg_begin.tag != vtag_int || arg_len.tag != vtag_int) {
        s->sp++;
        s->stack[s->sp] = make_value_none();

        return vm_status_pending;
    }

    ObjMutPtr src_ptr = heap_getm(&s->heap, arg.data.obj_id);

    if (!src_ptr) {
        s->sp++;
        s->stack[s->sp] = make_value_none();

        return vm_status_pending;
    }

    List *src_list = (List *)src_ptr;
    const size_t slice_base = arg_begin.data.i;
    const size_t slice_length = arg_len.data.i;

    if (slice_base + slice_length > src_list->data.length) {
        s->sp++;
        s->stack[s->sp] = make_value_none();

        return vm_status_pending;
    }

    // ! Here, Use the generic API to push the sliced out items to the result.
    ObjMutPtr result_ref = (ObjMutPtr)alloc_list(slice_length);

    if (!result_ref) {
        s->sp++;
        s->stack[s->sp] = make_value_none();

        return vm_status_err_abort;
    }

    for (size_t copy_offset = 0; copy_offset < slice_length; copy_offset++) {
        result_ref->set_v(result_ref, make_value_none(), src_list->data.data[slice_base + copy_offset]);
    }

    const int16_t reserved_obj_id = heap_store(&s->heap, result_ref);

    s->sp++;

    if (reserved_obj_id != DUD_HEAP_ID) {
        s->stack[s->sp] = make_value_obj(reserved_obj_id);
    } else {
        result_ref->del(result_ref);
        free(result_ref);
        s->stack[s->sp] = make_value_none(); 
    }
    
    return vm_status_pending;
}
