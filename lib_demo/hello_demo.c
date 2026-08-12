#include <stdio.h>

#include "tb_api.h"



#define DEMO_NATIVE_N 1

static VMStatus native_hello(VMState *s) {
    puts("Hello World!\n");

    s->sp++;
    s->stack[s->sp] = make_value_none(); // return NIL

    return vm_status_pending;
}

static const charspan extra_native_names[DEMO_NATIVE_N] = {
    (charspan) {
        .data = "hello",
        .length = 5
    },
};

static const NativeFn extra_native_fns[DEMO_NATIVE_N] = {
    native_hello,
};

int main(int argc, const char *argv[]) {
    // ? NOTE: run ./suite/lib_demo_tests/hello_world_2.tbasic
    return tbasic_run(argv, argc, extra_native_names, extra_native_fns, DEMO_NATIVE_N);
}
