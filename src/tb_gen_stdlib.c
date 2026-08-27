#include <math.h>
#include "obj_str.h"
#include "tb_gen_stdlib.h"



VMStatus native_powf(VMState *s) {
    const int callee_bp = s->bp;
    const Value a0 = s->stack[callee_bp + 1];
    const Value a1 = s->stack[callee_bp + 2];

    s->sp++;

    if (a0.tag != vtag_real || a1.tag != vtag_real) {
        s->stack[s->sp] = make_value_real(NAN);
    } else {
        s->stack[s->sp] = make_value_real(powf(a0.data.f, a1.data.f));
    }

    return vm_status_pending;
}

VMStatus native_sqrtf(VMState *s) {
    const int callee_bp = s->bp;
    const Value a0 = s->stack[callee_bp + 1];

    s->sp++;

    if (a0.tag != vtag_real) {
        s->stack[s->sp] = make_value_real(NAN);
    } else {
        s->stack[s->sp] = make_value_real(sqrtf(a0.data.f));
    }

    return vm_status_pending;
}

float clamp_f32(float v, float low, float high) {
    if (v < low) {
        return low;
    } else if (v > high) {
        return high;
    } else {
        return v;
    }
}

VMStatus native_clampf(VMState *s) {
    const int callee_bp = s->bp;
    const Value a0 = s->stack[callee_bp + 1];
    const Value a1 = s->stack[callee_bp + 2];
    const Value a2 = s->stack[callee_bp + 3];

    s->sp++;

    if (a0.tag != vtag_real || a1.tag != vtag_real || a2.tag != vtag_real) {
        s->stack[s->sp] = make_value_real(NAN);
    } else {
        s->stack[s->sp] = make_value_real(clamp_f32(a0.data.f, a1.data.f, a2.data.f));
    }

    return vm_status_pending;
}

VMStatus native_floorf(VMState *s) {
    const int callee_bp = s->bp;
    const Value a0 = s->stack[callee_bp + 1];

    s->sp++;

    if (a0.tag != vtag_real) {
        s->stack[s->sp] = make_value_real(NAN);
    } else {
        s->stack[s->sp] = make_value_real(floorf(a0.data.f));
    }

    return vm_status_pending;
}

VMStatus native_ceilf(VMState *s) {
    const int callee_bp = s->bp;
    const Value a0 = s->stack[callee_bp + 1];

    s->sp++;

    if (a0.tag != vtag_real) {
        s->stack[s->sp] = make_value_real(NAN);
    } else {
        s->stack[s->sp] = make_value_real(ceilf(a0.data.f));
    }

    return vm_status_pending;
}

VMStatus native_stoi(VMState *s) {
    const int callee_bp = s->bp;

    const Value arg = s->stack[callee_bp + 1];
    const char *str_chars = NULL;
    size_t str_length = 0;

    if (arg.tag == vtag_strid) {
        str_chars = AnyVec_mystr_get(&s->prgm->strings, arg.data.i)->data;
    } else if (arg.tag == vtag_obj_id) {
        ObjPtr temp = heap_get(&s->heap, arg.data.obj_id);

        if (temp != NULL && temp->meta.tag == otag_string) {
            str_chars = ((const String *)temp)->data.data;
            str_length = ((const String *)temp)->data.length;
        }
    }

    charspan sv = {
        .data = str_chars,
        .length = str_length
    };

    s->sp++;

    if (charspan_empty(&sv)) {
        s->stack[s->sp] = make_value_int(0);
    } else {
        s->stack[s->sp] = make_value_int(charspan_atoi(&sv));
    }

    return 1;
}

VMStatus native_stof(VMState *s) {
    const int callee_bp = s->bp;

    const Value arg = s->stack[callee_bp + 1];
    const char *str_chars = NULL;
    size_t str_length = 0;

    if (arg.tag == vtag_strid) {
        str_chars = AnyVec_mystr_get(&s->prgm->strings, arg.data.i)->data;
    } else if (arg.tag == vtag_obj_id) {
        ObjPtr temp = heap_get(&s->heap, arg.data.obj_id);

        if (temp != NULL && temp->meta.tag == otag_string) {
            str_chars = ((const String *)temp)->data.data;
            str_length = ((const String *)temp)->data.length;
        }
    }

    charspan sv = {
        .data = str_chars,
        .length = str_length
    };

    s->sp++;

    if (charspan_empty(&sv)) {
        s->stack[s->sp] = make_value_real(0.0f);
    } else {
        s->stack[s->sp] = make_value_real(charspan_checked_atof(&sv));
    }

    return 1;
}
