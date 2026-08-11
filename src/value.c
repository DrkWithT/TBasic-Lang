#include <stdio.h>
#include "value.h"
#include "vm.h"

void Value_dud(MAYBE_UNUSED Value* ins) {}
void Value_new(MAYBE_UNUSED Value *ins) {}
void Value_copy(MAYBE_UNUSED Value *ins, MAYBE_UNUSED const Value *other) {}
void Value_move(MAYBE_UNUSED Value *ins, MAYBE_UNUSED Value *other) {}
void Value_del(MAYBE_UNUSED Value *ins) {}



void print_string_k(const void *obj_ptr) {
    if (!obj_ptr) {
        puts("(unknown)");
        return;
    }

    const mystr *real_string = (const mystr *)obj_ptr;

    fwrite(
        mystr_raw(real_string),
        sizeof(char),
        mystr_len(real_string),
        stdout
    );
}

void print_object(const void *obj_ptr, const void *vm_state) {
    if (!vm_state) {
        puts("(unknown)");
        return;
    }

    ObjPtr obj_ref = (ObjPtr)obj_ptr;
    const VMState *vm = (const VMState *)vm_state;

    obj_ref->display(obj_ref, vm);
}

void print_value(const Value *v, const void *vm_state) {    
    const VMState *vm = (VMState *)vm_state;

    switch (v->tag) {
        case vtag_nil: printf("nil"); break;
        case vtag_bool: printf("%s", (v->data.byte) ? "TRUE" : "FALSE"); break;
        case vtag_int: printf("%d", v->data.i); break;
        case vtag_real: printf("%f", v->data.f); break;
        case vtag_strid: print_string_k(vm->prgm->strings.data + v->data.i); break;
        case vtag_obj_id: print_object(heap_get(&vm->heap, v->data.obj_id), vm); break;
        default: printf("(unknown)"); break;
    }
}

IMPL_VEC(Value)
