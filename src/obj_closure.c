#include "obj_closure.h"
#include "vm.h"



Closure *alloc_closure(const Chunk *bc, const Value *sub_stack, uint32_t n, uint16_t cid) {
    Closure *temp = ALLOC_TYPE(Closure);

    temp->base = (ObjBase) {
        .meta = {
            .tag = otag_closure,
            .flags = oflag_callable
        },
        .as_bool = closure_as_bool_fn,
        .get_v = closure_get_v_fn,
        .set_v = closure_set_v_fn,
        .display = closure_display_fn,
        .invoke = closure_invoke_fn
    };
    temp->bc = bc;

    Value *temp_upvals = NULL;
    DUD_SCALARS_N(Value, temp_upvals, n, __FILE__, __LINE__);
    memcpy(temp_upvals, sub_stack, sizeof(Value) * n);

    temp->upvals = temp_upvals;
    temp->count = n;
    temp->cid = cid;

    return temp;
}

void closure_del_fn(void *self) {
    Closure *closure = (Closure *)self;

    if (closure->upvals != NULL) {
        free(closure->upvals);
        closure->upvals = NULL;
        closure->count = 0;
    }

    closure->bc = NULL;
}

int8_t closure_as_bool_fn(const void *self) {
    return 1;
}

Value closure_get_v_fn(const void *self, Value key) {
    return make_value_none();
}

int8_t closure_set_v_fn(void *self, Value key, Value item) {
    return 0;
}

void closure_display_fn(const void *self, const void *vm) {
    Closure *closure = (Closure *)self;

    printf("Closure (bc-ptr = %p, value-ptr = %p)\n", closure->bc, closure->upvals);
}

uint8_t closure_invoke_fn(void *self, void *vm, const Instruction *caller_ip, const Value *caller_cvp, Value *stack_p, int16_t argc) {
    Closure *closure = (Closure *)self;
    VMState *s = (VMState *)vm;

    const Chunk *callee_chunk = closure->bc;
    const int caller_bp = s->bp;
    const int callee_bp = s->sp - argc;
    const uint16_t caller_cid = s->chunk_id;

    s->bp = callee_bp;
    s->chunk_id = closure->cid;
    s->upvals = closure->upvals;

    // ? For speed and simplicity, use the native stack to track VM call recursions...
    s->depth++;

    const VMStatus closure_call_status = vm_dispatch(s, closure->bc->code.data, closure->bc->constants.data, stack_p);

    stack_p[callee_bp] = stack_p[s->sp];
    s->sp = callee_bp;
    s->bp = caller_bp;
    s->chunk_id = caller_cid;
    s->upvals = NULL;

    if (s->depth == 0) {
        return s->status;
    }

    return closure_call_status;
}
