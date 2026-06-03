#include <math.h>
#include "vm.h"
#include "obj_list.h"
#include "obj_str.h"
#include "obj_dict.h"


static const OpFunc opcode_handlers[] = {
    fn_nop,
    fn_put_none,
    fn_put_bool,
    fn_load_imm_gid,
    fn_load_local,
    fn_store_local,
    fn_put_k,
    fn_dup,
    fn_pop,
    fn_load_string_k,
    fn_mk_list,
    fn_mk_dict,
    fn_get_idx,
    fn_set_idx,
    fn_chk_none,
    fn_mul,
    fn_div,
    fn_add,
    fn_sub,
    fn_eq,
    fn_ne,
    fn_lt,
    fn_gt,
    fn_jmp,
    fn_jmp_false,
    fn_jmp_if,
    fn_call,
    fn_put_callee,
    fn_ret
};

VMStatus fn_nop(VMState *s, const Instruction *ip, const Value* cvp, Value *stack) {
    ip++;

    TAILCALL
    return vm_dispatch(s, ip, cvp, stack);
}

VMStatus fn_put_none(VMState *s, const Instruction *ip, const Value *cvp, Value *stack) {
    s->sp++;
    stack[s->sp] = make_value_none();
    ip++;

    TAILCALL
    return vm_dispatch(s, ip, cvp, stack);
}

VMStatus fn_put_bool(VMState *s, const Instruction *ip, const Value *cvp, Value *stack) {
    s->sp++;
    stack[s->sp] = make_value_bool(ip->flag & 1);
    ip++;

    TAILCALL
    return vm_dispatch(s, ip, cvp, stack);
}

VMStatus fn_load_imm_gid(VMState *s, const Instruction *ip, const Value *cvp, Value *stack) {
    s->sp++;
    stack[s->sp] = make_value_int(ip->wide); // ? push ID of procedure's chunk
    ip++;

    TAILCALL
    return vm_dispatch(s, ip, cvp, stack);
}

VMStatus fn_load_local(VMState *s, const Instruction *ip, const Value *cvp, Value *stack) {
    s->sp++;
    stack[s->sp] = stack[s->bp + ip->wide];
    ip++;

    TAILCALL
    return vm_dispatch(s, ip, cvp, stack);
}

VMStatus fn_store_local(VMState *s, const Instruction *ip, const Value *cvp, Value *stack) {
    stack[s->bp + ip->wide] = stack[s->sp];
    s->sp--;
    ip++;

    TAILCALL
    return vm_dispatch(s, ip, cvp, stack);
}

VMStatus fn_put_k(VMState *s, const Instruction *ip, const Value *cvp, Value *stack) {
    s->sp++;
    stack[s->sp] = cvp[ip->wide];
    ip++;

    TAILCALL
    return vm_dispatch(s, ip, cvp, stack);
}

VMStatus fn_dup(VMState *s, const Instruction *ip, const Value *cvp, Value *stack) {
    s->sp++;
    stack[s->sp] = cvp[s->sp - 1];
    ip++;

    TAILCALL
    return vm_dispatch(s, ip, cvp, stack);
}

VMStatus fn_pop(VMState *s, const Instruction *ip, const Value *cvp, Value *stack) {
    s->sp -= ip->flag;
    ip++;

    TAILCALL
    return vm_dispatch(s, ip, cvp, stack);
}

VMStatus fn_load_string_k(VMState *s, const Instruction *ip, const Value *cvp, Value *stack) {
    s->sp++;
    stack[s->sp] = make_value_str(ip->wide);
    ip++;

    TAILCALL
    return vm_dispatch(s, ip, cvp, stack);
}

VMStatus fn_mk_list(VMState *s, const Instruction *ip, const Value *cvp, Value *stack) {
    GCState_collect(&s->gc, &s->heap, stack, s->sp);

    const size_t pushing_count = ip->wide;
    ObjMutPtr temp_object = (ObjMutPtr)alloc_list(pushing_count);

    if (!temp_object) {
        s->status = vm_status_err_abort;
        return s->status;
    }

    const int16_t temp_object_id = heap_store(&s->heap, temp_object);

    /*
     * Example: Push items 1 to 3 inclusively and in-order to temp_object...
     * SAMPLE OPCODE: MK_LIST (N = 3)
     * 
     * -- BEFORE --------------------
     * |  item 3  | <-- SP
     * |  item 2  |
     * |  item 1  | <-- BASE_ITEM_POS
     *
     * -- AFTER ---------------------
     * | (popped) |
     * | (popped) |
     * | (popped) |
     * | <obj ID> | <-- SP = OLD_SP - N + 1
     */
    const size_t base_item_pos = s->sp - pushing_count + 1;
    for (int offset = 0; offset < pushing_count; offset++) {
        temp_object->set_v(temp_object, make_value_none(), stack[base_item_pos + offset]); // ? list[nil] := temp; is like list.push_back(item);
    }

    s->sp -= (pushing_count - 1);
    stack[s->sp] = make_value_obj(temp_object_id);
    ip++;

    TAILCALL
    return vm_dispatch(s, ip, cvp, stack);
}

VMStatus fn_mk_dict(VMState *s, const Instruction *ip, const Value *cvp, Value *stack) {
    GCState_collect(&s->gc, &s->heap, stack, s->sp);

    ObjMutPtr temp_object = (ObjMutPtr)alloc_dict();

    if (!temp_object) {
        s->status = vm_status_err_abort;
        return s->status;
    }

    const int16_t temp_object_id = heap_store(&s->heap, temp_object);

    s->sp++;
    stack[s->sp] = make_value_obj(temp_object_id);
    ip++;

    TAILCALL
    return vm_dispatch(s, ip, cvp, stack);
}

VMStatus fn_get_idx(VMState *s, const Instruction *ip, const Value *cvp, Value *stack) {
    /*
     * EXAMPLE: expr foo::0 ;
     *
     * ------ BEFORE --------------------------------
     * | Value(int(0)) | <-- SP, i32 of 0 as index
     * | Value(oid(X)) | <-- object ID as "reference"
     *
     * ------ AFTER ---------------------------------
     * |   (popped!)   |
     * | Value(foo[0]) | <-- SP
     */
    const int16_t target_object_id = (stack[s->sp - 1].tag == vtag_obj_id)
        ? stack[s->sp - 1].data.obj_id
        : DUD_HEAP_ID;
    ObjPtr object_ref = heap_get(&s->heap, target_object_id);

    if (!object_ref) {
        s->status = vm_status_err_bad_op;
        return s->status;
    } else if (object_ref->meta.tag == otag_dud) {
        s->status = vm_status_err_bad_op;
        return s->status;
    } else {
        s->sp--;
        stack[s->sp] = object_ref->get_v(object_ref, stack[s->sp + 1]);
    }

    ip++;

    TAILCALL
    return vm_dispatch(s, ip, cvp, stack);
}

VMStatus fn_set_idx(VMState *s, const Instruction *ip, const Value *cvp, Value *stack) {
    /*
     * EXAMPLE: expr foo[0] := 69420;
     *
     * ------ BEFORE --------------------------------
     * | Value(int(69420)) | <-- SP - 0, temporary to store
     * | Value(int(0))     | <-- SP - 1, i32 of 0 as index
     * | Value(oid(X))     | <-- object ID as "reference"
     *
     * ------ AFTER ---------------------------------
     * | (popped!)          |
     * | (popped!)          |
     * | Value(oid(X))  | <-- SP, assignment leaves the same object, allowing (foo::0 := 1) + 2??
     */
    
    const Value incoming_temp = stack[s->sp];
    const int16_t target_object_id = (stack[s->sp - 2].tag == vtag_obj_id)
        ? stack[s->sp - 2].data.obj_id
        : DUD_HEAP_ID;
    ObjMutPtr object_ref = heap_getm(&s->heap, target_object_id);

    if (!object_ref) {
        s->status = vm_status_err_bad_op;
        return s->status;
    } else if (object_ref->meta.tag == otag_dud) {
        s->status = vm_status_err_bad_op;
        return s->status;
    } else {
        object_ref->set_v(object_ref, stack[s->sp - 1], incoming_temp);
        s->sp -= 2;
        // stack[s->sp] = incoming_temp;
    }

    ip++;

    TAILCALL
    return vm_dispatch(s, ip, cvp, stack);
}

VMStatus fn_chk_none(VMState *s, const Instruction *ip, const Value *cvp, Value *stack) {
    const int sp = s->sp;

    stack[sp + 1] = make_value_bool(stack[sp].tag == vtag_nil);
    s->sp++;
    ip++;

    TAILCALL
    return vm_dispatch(s, ip, cvp, stack);
}

VMStatus fn_mul(VMState *s, const Instruction *ip, const Value *cvp, Value *stack) {
    Value *lhs = stack + s->sp - 1;
    const Value *rhs = stack + s->sp;

    if (lhs->tag != rhs->tag) {
        *lhs = make_value_real(NAN);
    } else {
        switch (lhs->tag) {
        case vtag_int:
            *lhs = make_value_int(lhs->data.i * rhs->data.i);
            break;
        case vtag_real:
            *lhs = make_value_real(lhs->data.f * rhs->data.f);
            break;
        default:
            *lhs = make_value_real(NAN);
            break;
        }
    }

    s->sp--;
    ip++;

    TAILCALL
    return vm_dispatch(s, ip, cvp, stack);
}

VMStatus fn_div(VMState *s, const Instruction *ip, const Value *cvp, Value *stack) {
    Value *lhs = stack + s->sp - 1;
    const Value *rhs = stack + s->sp;

    if (lhs->tag != rhs->tag) {
        *lhs = make_value_real(NAN);
    } else {
        switch (lhs->tag) {
        case vtag_int:
            *lhs = (rhs->data.i != 0)
                ? make_value_int(lhs->data.i / rhs->data.i)
                : make_value_real(NAN);
            break;
        case vtag_real:
            *lhs = (rhs->data.f != 0.0f)
                ? make_value_real(lhs->data.f / rhs->data.f)
                : make_value_real(NAN);
            break;
        default:
            *lhs = make_value_real(NAN);
            break;
        }
    }

    s->sp--;
    ip++;

    TAILCALL
    return vm_dispatch(s, ip, cvp, stack);
}

VMStatus fn_add(VMState *s, const Instruction *ip, const Value *cvp, Value *stack) {
    Value *lhs = stack + s->sp - 1;
    const Value *rhs = stack + s->sp;

    if (lhs->tag != rhs->tag) {
        *lhs = make_value_real(NAN);
    } else {
        switch (lhs->tag) {
        case vtag_int:
            *lhs = make_value_int(lhs->data.i + rhs->data.i);
            break;
        case vtag_real:
            *lhs = make_value_real(lhs->data.f + rhs->data.f);
            break;
        default:
            *lhs = make_value_real(NAN);
            break;
        }
    }

    s->sp--;
    ip++;

    TAILCALL
    return vm_dispatch(s, ip, cvp, stack);
}

VMStatus fn_sub(VMState *s, const Instruction *ip, const Value *cvp, Value *stack) {
    Value *lhs = stack + s->sp - 1;
    const Value *rhs = stack + s->sp;

    if (lhs->tag != rhs->tag) {
        *lhs = make_value_real(NAN);
    } else {
        switch (lhs->tag) {
        case vtag_int:
            *lhs = make_value_int(lhs->data.i - rhs->data.i);
            break;
        case vtag_real:
            *lhs = make_value_real(lhs->data.f - rhs->data.f);
            break;
        default:
            *lhs = make_value_real(NAN);
            break;
        }
    }

    s->sp--;
    ip++;

    TAILCALL
    return vm_dispatch(s, ip, cvp, stack);
}

VMStatus fn_eq(VMState *s, const Instruction *ip, const Value *cvp, Value *stack) {
    Value *lhs = stack + s->sp - 1;
    const Value *rhs = stack + s->sp;

    if (lhs->tag != rhs->tag) {
        *lhs = make_value_bool(0);
    } else {
        switch (lhs->tag) {
        case vtag_nil:
            *lhs = make_value_bool(1);
            break;
        case vtag_bool:
            *lhs = make_value_bool(lhs->data.byte == rhs->data.byte);
            break;
        case vtag_int:
            *lhs = make_value_bool(lhs->data.i == rhs->data.i);
            break;
        case vtag_real:
            *lhs = make_value_bool(lhs->data.f == rhs->data.f);
            break;
        default:
            *lhs = make_value_bool(0);
            break;
        }
    }

    s->sp--;
    ip++;

    TAILCALL
    return vm_dispatch(s, ip, cvp, stack);
}

VMStatus fn_ne(VMState *s, const Instruction *ip, const Value *cvp, Value *stack) {
    Value *lhs = stack + s->sp - 1;
    const Value *rhs = stack + s->sp;

    if (lhs->tag != rhs->tag) {
        *lhs = make_value_bool(0);
    } else {
        switch (lhs->tag) {
        case vtag_nil:
            *lhs = make_value_bool(0);
            break;
        case vtag_bool:
            *lhs = make_value_bool(lhs->data.byte != rhs->data.byte);
            break;
        case vtag_int:
            *lhs = make_value_bool(lhs->data.i != rhs->data.i);
            break;
        case vtag_real:
            *lhs = make_value_bool(lhs->data.f != rhs->data.f);
            break;
        default:
            *lhs = make_value_bool(0);
            break;
        }
    }

    s->sp--;
    ip++;

    TAILCALL
    return vm_dispatch(s, ip, cvp, stack);
}

VMStatus fn_lt(VMState *s, const Instruction *ip, const Value *cvp, Value *stack) {
    Value *lhs = stack + s->sp - 1;
    const Value *rhs = stack + s->sp;

    if (lhs->tag != rhs->tag) {
        *lhs = make_value_bool(0);
    } else {
        switch (lhs->tag) {
        case vtag_nil:
            *lhs = make_value_bool(0);
            break;
        case vtag_bool:
            *lhs = make_value_bool(lhs->data.byte < rhs->data.byte);
            break;
        case vtag_int:
            *lhs = make_value_bool(lhs->data.i < rhs->data.i);
            break;
        case vtag_real:
            *lhs = make_value_bool(lhs->data.f < rhs->data.f);
            break;
        default:
            *lhs = make_value_bool(0);
            break;
        }
    }

    s->sp--;
    ip++;

    TAILCALL
    return vm_dispatch(s, ip, cvp, stack);
}

VMStatus fn_gt(VMState *s, const Instruction *ip, const Value *cvp, Value *stack) {
    Value *lhs = stack + s->sp - 1;
    const Value *rhs = stack + s->sp;

    if (lhs->tag != rhs->tag) {
        *lhs = make_value_bool(0);
    } else {
        switch (lhs->tag) {
        case vtag_nil:
            *lhs = make_value_bool(0);
            break;
        case vtag_bool:
            *lhs = make_value_bool(lhs->data.byte > rhs->data.byte);
            break;
        case vtag_int:
            *lhs = make_value_bool(lhs->data.i > rhs->data.i);
            break;
        case vtag_real:
            *lhs = make_value_bool(lhs->data.f > rhs->data.f);
            break;
        default:
            *lhs = make_value_bool(0);
            break;
        }
    }

    s->sp--;
    ip++;

    TAILCALL
    return vm_dispatch(s, ip, cvp, stack);
}

VMStatus fn_jmp(VMState *s, const Instruction *ip, const Value *cvp, Value *stack) {
    if (ip->flag) {
        // ? If IP->FLAG == 1, the jump is negative (backwards).
        ip -= ip->wide;
    } else {
        ip += ip->wide;
    }
        
    TAILCALL
    return vm_dispatch(s, ip, cvp, stack);
}

VMStatus fn_jmp_false(VMState *s, const Instruction *ip, const Value *cvp, Value *stack) {
    const Value *temp = stack + s->sp;
    ObjPtr temp_as_obj = NULL;
    int8_t require_truthy_pop = 0;

    switch (temp->tag) {
    case vtag_nil: break;
    case vtag_bool:
        require_truthy_pop = temp->data.byte != 0;
        break;
    case vtag_int:
        require_truthy_pop = temp->data.i != 0;
        break;
    case vtag_real:
        require_truthy_pop = temp->data.f != 0.0f;
        break;
    case vtag_obj_id:
        temp_as_obj = heap_get(&s->heap, (temp->tag == vtag_obj_id) ? temp->data.obj_id : -1); // ? Use polymorphic as_bool() call on the object ONLY IF it's legit... For safety reasons.
        require_truthy_pop = (temp_as_obj) ? temp_as_obj->as_bool(temp_as_obj) : 0;
        break;
    default:
        break;
    }

    // ? NOTE: IF temp == TRUE, POP it & advance to next evaluation. This works for short-circuiting of `temp_eval1 --> LHS && temp_eval2 --> RHS`.
    if (require_truthy_pop) {
        s->sp--;
        ip++;
    } else {
        ip += ip->wide;
    }

    TAILCALL
    return vm_dispatch(s, ip, cvp, stack);
}

VMStatus fn_jmp_if(VMState *s, const Instruction *ip, const Value *cvp, Value *stack) {
    const Value *temp = stack + s->sp;

    switch (temp->tag) {
    case vtag_nil:
        ip++;
        break;
    case vtag_bool:
        ip += (temp->data.byte != 0) ? ip->wide : 1;
        break;
    case vtag_int:
        ip += (temp->data.i != 0) ? ip->wide : 1;
        break;
    case vtag_real:
        ip += (temp->data.f != 0.0f) ? ip->wide : 1;
        break;
    default:
        s->sp--;
        ip++;
        break;
    }

    TAILCALL
    return vm_dispatch(s, ip, cvp, stack);
}

VMStatus fn_call(VMState *s, const Instruction *ip, const Value *cvp, Value *stack) {
    const int16_t arg_count = ip->wide;

    if (ip->flag && stack[s->sp - arg_count].tag == vtag_int) {
        s->status = s->native_table[stack[s->sp - arg_count].data.i](s, ip->wide);
        ip++;

        TAILCALL
        return vm_dispatch(s, ip, cvp, stack);
    } else if (stack[s->sp - arg_count].tag != vtag_int) {
        return vm_status_err_bad_call;
    }
    
    const Chunk *callee_chunk = s->prgm->chunks.data + stack[s->sp - arg_count].data.i;
    const Instruction *caller_ret_ip = ip + 1;
    const Value *caller_cvp = cvp;
    int caller_bp = s->bp;
    int callee_bp = s->sp - arg_count;

    s->bp = callee_bp;

    // ? For speed and simplicity, use the native stack to track VM call recursions...
    s->depth++;
    vm_dispatch(s, callee_chunk->code.data, callee_chunk->constants.data, stack);

    stack[callee_bp] = stack[s->sp];
    s->sp = callee_bp;
    s->bp = caller_bp;
    ip = caller_ret_ip;
    cvp = caller_cvp;

    if (s->depth == 0) {
        return s->status;
    }

    TAILCALL
    return vm_dispatch(s, ip, cvp, stack);
}

VMStatus fn_put_callee(VMState *s, const Instruction *ip, const Value *cvp, Value *stack) {
    s->sp++;
    stack[s->sp] = stack[s->bp]; // ? assume a function ID is always at CALLEE_BP + 0
    ip++;

    TAILCALL
    return vm_dispatch(s, ip, cvp, stack);
}

VMStatus fn_ret(VMState *s, const Instruction *ip, const Value *cvp, Value *stack) {
    s->depth--;

    if (s->depth < 1) {
        if (s->status == vm_status_pending) {
            s->status = vm_status_ok;
        }

        stack[0] = stack[s->sp];
        s->sp = 0;
        s->bp = 0;
    }

    return s->status;
}

VMStatus vm_dispatch(VMState *s, const Instruction *ip, const Value *cvp, Value *stack) {
    TAILCALL
    return opcode_handlers[ip->op](s, ip, cvp, stack);
}



VMState make_vm(const Program *program, const NativeFn *native_table_ptr, int locals_max, uint8_t depth_max, int16_t heap_pop_max) {
    const Chunk *entry_chunk = program->chunks.data + program->entry_id;
    Value *stack_buffer = calloc(locals_max, sizeof(Value));

    ObjHeap temp_heap;
    heap_dud(&temp_heap); // TODO: Later, make VM creation take an initial heap capacity. This actually defaults to DEFAULT_GC_CAPACITY (256) object cells.

    GCState temp_gc;
    GCState_new(&temp_gc, heap_pop_max);

    return (VMState) {
        .heap = temp_heap,
        .gc = temp_gc,
        .native_table = native_table_ptr,
        .prgm = program,
        .ip = entry_chunk->code.data,
        .cvp = entry_chunk->constants.data,
        .stack = stack_buffer,
        .sp = 0,
        .bp = 0,
        .depth = 1,
        .status = (stack_buffer != NULL) ? vm_status_pending : vm_status_err_abort
    };
}

void dispose_vm(VMState *s) {
    heap_del(&s->heap);
    GCState_del(&s->gc);

    if (s->stack != NULL) {
        free(s->stack);
        s->stack = NULL;
    }
}

VMStatus vm_status(const VMState *s) {
    return s->status;
}

Value vm_result(const VMState *s) {
    return s->stack[0];
}

VMStatus vm_run(VMState *s) {
    return vm_dispatch(s, s->ip, s->cvp, s->stack);
}

int16_t vm_put_heap_string(VMState *s, mystr *string) {
    if (string == NULL) {
        return 0;
    }

    return heap_store(&s->heap, (ObjMutPtr)alloc_string_of_mystr(string));
}
