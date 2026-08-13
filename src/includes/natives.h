#ifndef TBASIC_NATIVES_H
#define TBASIC_NATIVES_H



#include <stdio.h>
#include <math.h>

#include "mystr.h"
// #include "obj_list.h"
#include "obj_str.h"
#include "vm.h"



static inline void native_print_str(const mystr *strings, int id) {
    if (id < 0) {
        printf("'...'");
    } else {
        printf("%s", strings[id].data);
    }
}

/*
 * Invariants: 
 * 1. Returns NONE in STACK[CALLEE_BP].
 * 2. The convention is followed for the VM:
 *      CALLEE_BP = SP - ARGC
 *      LOCALS[N] = STACK[CALLEE_BP + 1 + N]
 * Stack Layout: of print(1, 2, 3)
 * | Value(Int(3)) | <--- SP <--- CALLEE_BP + 3
 * | Value(Int(2)) |
 * | Value(Int(1)) | <--- LOCAL_1 <--- CALLEE_BP + 1
 * | Value(Fun-ID) | <--- CALLEE_BP = SP - ARGC = SP - 3 <--- PUT "none"
 */
static inline VMStatus native_print(VMState *s) {
    const int callee_bp = s->bp;
    const int argc = s->sp - s->bp;

    for (int i = 1; i <= argc; i++) {
        const Value *arg_ref = s->stack + callee_bp + i;

        print_value(arg_ref, s);
        printf(" ");
    }

    printf("\n");

    s->sp++;
    s->stack[s->sp] = make_value_none();

    return vm_status_pending;
}

static inline VMStatus native_powf(VMState *s) {
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

static inline VMStatus native_sqrtf(VMState *s) {
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

static inline float clamp_f32(float v, float low, float high) {
    if (v < low) {
        return low;
    } else if (v > high) {
        return high;
    } else {
        return v;
    }
}

static inline VMStatus native_clampf(VMState *s) {
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

static inline VMStatus native_floorf(VMState *s) {
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

static inline VMStatus native_ceilf(VMState *s) {
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

static inline VMStatus native_console_readln(VMState *s) {
    const int callee_bp = s->bp;

    if (feof(stdin)) {
        clearerr(stdin);
    } else if (ferror(stdin)) {
        fprintf(stderr, "STDIN is in an errorneous state, try console_reset().\n");
        return 0;
    }

    mystr input_str;
    mystr_new(&input_str, "");
    char c = '\0';
    size_t rc = 0;

    while (1) {
        rc = fread(&c, sizeof(char), 1, stdin);

        if (c == '\n' || rc <= 0 || feof(stdin)) {
            break;
        } else if (ferror(stdin)) {
            perror("Failed to read line.");
            break;
        } else {
            mystr_append_raw(&input_str, &c, 1);
        }
    }

    s->sp++;
    s->stack[s->sp] = make_value_obj(vm_put_heap_string(s, &input_str));

    return 1;
}

static inline VMStatus native_console_reset(VMState *s) {
    const int callee_bp = s->bp;

    clearerr(stdin);

    s->sp++;
    s->stack[s->sp] = make_value_none();

    return 1;
}

static inline VMStatus native_stoi(VMState *s) {
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

static inline VMStatus native_stof(VMState *s) {
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

#endif
