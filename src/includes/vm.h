#ifndef TBASIC_VM_H
#define TBASIC_VM_H

#include <stdint.h>
#include "bytecode.h"
#include "objects.h"
#include "gc.h"


#define TAILCALL __attribute((musttail))

typedef enum vm_status_t : uint8_t {
    vm_status_ok,
    vm_status_pending,
    vm_status_err_bad_op,
    vm_status_err_bad_call,
    vm_status_err_abort
} VMStatus;

// ? Provides a forward decl. of VMState:
typedef struct vm_state_t VMState;

// ? Provides a type alias for native built-in functions:
typedef VMStatus(*NativeFn)(VMState *s, int argc);

// ? This opcode handler type MUST do at least 2 things: update IP & tail-call into the dispatch function.
typedef VMStatus(*OpFunc)(VMState *, const Instruction *, const Value *, Value *);

VMStatus fn_nop(VMState *, const Instruction *, const Value *, Value *);
VMStatus fn_put_none(VMState *, const Instruction *, const Value *, Value *);
VMStatus fn_put_bool(VMState *, const Instruction *, const Value *, Value *);
VMStatus fn_load_imm_gid(VMState *, const Instruction *, const Value *, Value *);
VMStatus fn_load_local(VMState *, const Instruction *, const Value *, Value *);
VMStatus fn_store_local(VMState *, const Instruction *, const Value *, Value *);
VMStatus fn_put_k(VMState *, const Instruction *, const Value *, Value *);
VMStatus fn_dup(VMState *, const Instruction *, const Value *, Value *);
VMStatus fn_pop(VMState *, const Instruction *, const Value *, Value *);
VMStatus fn_load_string_k(VMState *s, const Instruction *ip, const Value *cvp, Value *stack);
VMStatus fn_mk_list(VMState *, const Instruction *, const Value *, Value *);
VMStatus fn_mk_dict(VMState *, const Instruction *, const Value *, Value *);
VMStatus fn_get_idx(VMState *, const Instruction *, const Value *, Value *);
VMStatus fn_set_idx(VMState *, const Instruction *, const Value *, Value *);
VMStatus fn_chk_none(VMState *, const Instruction *, const Value *, Value *);
VMStatus fn_mul(VMState *, const Instruction *, const Value *, Value *);
VMStatus fn_div(VMState *, const Instruction *, const Value *, Value *);
VMStatus fn_add(VMState *, const Instruction *, const Value *, Value *);
VMStatus fn_sub(VMState *, const Instruction *, const Value *, Value *);
VMStatus fn_eq(VMState *, const Instruction *, const Value *, Value *);
VMStatus fn_ne(VMState *, const Instruction *, const Value *, Value *);
VMStatus fn_lt(VMState *, const Instruction *, const Value *, Value *);
VMStatus fn_gt(VMState *, const Instruction *, const Value *, Value *);
VMStatus fn_jmp(VMState *, const Instruction *, const Value *, Value *);
VMStatus fn_jmp_false(VMState *, const Instruction *, const Value *, Value *);
VMStatus fn_jmp_if(VMState *, const Instruction *, const Value *, Value *);
VMStatus fn_call(VMState *, const Instruction *, const Value *, Value *);
VMStatus fn_put_callee(VMState *, const Instruction *, const Value *, Value *);
VMStatus fn_ret(VMState *, const Instruction *, const Value *, Value *);
VMStatus vm_dispatch(VMState *s, const Instruction *ip, const Value *cvp, Value *stack);



// ? Main VM state. Tracks stack state and refers to a current bytecode chunk for dispatch.
typedef struct vm_state_t {
    ObjHeap heap;
    GCState gc;
    const NativeFn *native_table;
    const Program *prgm;
    const Instruction *ip;
    const Value *cvp;
    Value *stack;
    int sp;
    int bp;
    uint8_t depth;
    VMStatus status;
} VMState;

VMState make_vm(const Program *program, const NativeFn *native_table_ptr, int locals_max, uint8_t depth_max, int16_t heap_pop_max);

void dispose_vm(VMState *s);

VMStatus vm_status(const VMState *s);

Value vm_result(const VMState *s);

VMStatus vm_run(VMState *s);

int16_t vm_put_heap_string(VMState *s, mystr *string);

#endif
