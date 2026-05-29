#ifndef TBASIC_API_H
#define TBASIC_API_H

#include <stdio.h>

#include "mystr.h"
#include "natives.h"
#include "driver.h"


/**
 * @brief This is the main routine which sets up and runs the TBasic interpreter, exposed as the one API item.
 * 
 * @param argv --- `main()` argument strings
 * @param argc --- `main()` argument count, including process path
 * @param native_names --- N-sized array of `charspan`s, each viewing a static lifetime `const char *`
 * @param native_fns --- N-sized array of function pointer type `VMStatus (*)(VMState *, int)`
 * @param n --- Length of `native_names` and `native_fns`.
 * @return int --- `0` on success, `1` on failure.
 */
static inline int tbasic_run(const char *argv[], int argc, const charspan *native_names, const NativeFn *native_fns, int n) {
    const char *source_fpath = NULL;
    int8_t show_info = 0;
    int8_t dump_bc = 0;
    int8_t allow_run = 0;

    if (argc < 2) {
        fprintf(stderr, "Too few arguments, try ./tbasic -i for information.\n");
        return 1;
    }

    for (int16_t arg_i = 0; arg_i < argc - 1; arg_i++) {
        if (!strcmp(argv[1 + arg_i], "-i")) { show_info = 1;}
        else if (!strcmp(argv[1 + arg_i], "-d")) { dump_bc = 1; }
        else if (!strcmp(argv[1 + arg_i], "-r")) { allow_run = 1; }
        else { source_fpath = argv[1 + arg_i]; }
    }

    DriverConfig config = {
        .title = " _____ _____         _     \n"
                 "|_   _| __  |___ ___|_|___ \n"
                 "  | | | __ -| .'|_ -| |  _|\n"
                 "  |_| |_____|__,|___|_|___|\n",
        .stack_capacity = CONFIG_DEFAULT_VM_LOCALS,
        .heap_capacity = CONFIG_DEFAULT_VM_HEAP_POPULATION,
        .recursion_max = CONFIG_DEFAULT_VM_RECUR_LIMIT,
        .version_major = 0,
        .version_minor = 6,
        .version_patch = 3
    };

    Driver app;
    driver_dud(&app, &config);

    driver_set_flag(&app, dflag_info, show_info);
    driver_set_flag(&app, dflag_dis_bc, dump_bc);
    driver_set_flag(&app, dflag_run_bc, allow_run);

    // TODO: add more library functions for time, math, I/O.
    driver_bind_native(&app, (charspan) {.data = "print", .length = 5}, native_print);
    driver_bind_native(&app, (charspan) {.data = "powf", .length = 4}, native_powf);
    driver_bind_native(&app, (charspan) {.data = "sqrtf", .length = 5}, native_sqrtf);
    driver_bind_native(&app, (charspan) {.data = "clampf", .length = 6}, native_clampf);
    driver_bind_native(&app, (charspan) {.data = "floorf", .length = 6}, native_floorf);
    driver_bind_native(&app, (charspan) {.data = "ceilf", .length = 5}, native_ceilf);
    driver_bind_native(&app, (charspan) {.data = "creadln", .length = 7}, native_console_readln);
    driver_bind_native(&app, (charspan) {.data = "creset", .length = 6}, native_console_reset);
    driver_bind_native(&app, (charspan) {.data = "stoi", .length = 4}, native_stoi);
    driver_bind_native(&app, (charspan) {.data = "stof", .length = 4}, native_stof);

    if (n < 1) {
        n = 0;
    }

    for (int i = 0; i < n; i++) {
        driver_bind_native(&app, native_names[i], native_fns[i]);
    }

    return driver_run(&app, source_fpath);
}



static inline VMStatus tbasic_get_status(const VMState *vm) {
    return vm->status;
}

static inline void tbasic_set_status(VMState *vm, VMStatus status) {
    vm->status = status;
}

static inline int tbasic_pop_n(VMState *vm, uint8_t n) {
    vm->sp -= n;

    return vm->sp;
}

static inline int tbasic_push_nil(VMState *vm) {
    vm->sp++;
    vm->stack[vm->sp] = make_value_none();

    return vm->sp;
}

static inline int tbasic_push_bool(VMState *vm, int8_t b) {
    vm->sp++;
    vm->stack[vm->sp] = make_value_bool(b);

    return vm->sp;
}

static inline int tbasic_push_int(VMState *vm, int i) {
    vm->sp++;
    vm->stack[vm->sp] = make_value_int(i);

    return vm->sp;
}

static inline int tbasic_push_float(VMState *vm, float f) {
    vm->sp++;
    vm->stack[vm->sp] = make_value_real(f);

    return vm->sp;
}

static inline int tbasic_push_string(VMState *vm, const char *cstr, size_t n) {
    mystr temp_s;
    mystr_res(&temp_s, DEFAULT_STRING_SIZE);

    if (!mystr_append_raw(&temp_s, cstr, n)) {
        FATAL_ABORT("TBAPI ERROR", __FILE__, __LINE__, "Failed to fill mystr for passed string of const char *cstr, allocation may have failed.");
    }

    String *s_object = alloc_string_of_mystr(&temp_s);

    if (!s_object) {
        FATAL_ABORT("TBAPI ERROR", __FILE__, __LINE__, "Failed to create TBasic string, allocation may have failed.");
    }

    const int16_t s_object_id = heap_store(&vm->heap, (ObjMutPtr)s_object);

    vm->sp++;
    vm->stack[vm->sp] = make_value_obj(s_object_id);

    return vm->sp;
}

#endif