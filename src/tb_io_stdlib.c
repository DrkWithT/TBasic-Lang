#include <stdio.h>
#include "tb_io_stdlib.h"



void native_print_str(const mystr *strings, int id) {
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
VMStatus native_print(VMState *s) {
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

VMStatus native_console_readln(VMState *s) {
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

VMStatus native_console_reset(VMState *s) {
    const int callee_bp = s->bp;

    clearerr(stdin);

    s->sp++;
    s->stack[s->sp] = make_value_none();

    return 1;
}
