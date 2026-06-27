#ifndef TBASIC_API_H
#define TBASIC_API_H

#include <stdio.h>

#include "mystr.h"
#include "natives.h"
#include "load_bc.h"
#include "driver.h"



static inline Program tbasic_deserialize_prgm(const uint8_t *data, size_t n) {
    TBLoader loader;
    tbl_dud(&loader);

    return tbl_load_from(&loader, data, n);
}

static inline int8_t tbasic_validate_prgm(const Program *pg) {
    if (pg->chunks.data == NULL && pg->strings.data == NULL) {
        return 0;
    } else if (pg->entry_id < 0 || pg->entry_id >= pg->chunks.length) {
        return 0;
    }

    return 1;
}

static inline Driver tbasic_make_driver(const charspan *native_names, const NativeFn *native_fns, int n, Program *pg) {
    DriverConfig config = {
        .title = " _____ _____         _     \n"
                 "|_   _| __  |___ ___|_|___ \n"
                 "  | | | __ -| .'|_ -| |  _|\n"
                 "  |_| |_____|__,|___|_|___|\n",
        .stack_capacity = CONFIG_DEFAULT_VM_LOCALS,
        .heap_capacity = CONFIG_DEFAULT_VM_HEAP_POPULATION,
        .recursion_max = CONFIG_DEFAULT_VM_RECUR_LIMIT,
        .version_major = TBASIC_VERSION_MAJOR,
        .version_minor = TBASIC_VERSION_MINOR,
        .version_patch = TBASIC_VERSION_PATCH
    };

    Driver app;
    driver_dud(&app, &config);

    driver_set_flag(&app, dflag_run_bc, 1);

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

    app.vm = make_vm(pg, app.natives.data, app.config.stack_capacity, app.config.recursion_max, app.config.heap_capacity);

    return app;
}

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
        .version_major = TBASIC_VERSION_MAJOR,
        .version_minor = TBASIC_VERSION_MINOR,
        .version_patch = TBASIC_VERSION_PATCH
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

    const int exit_code = driver_run(&app, source_fpath);

    driver_del(&app);
    return exit_code;
}



static inline VMStatus tbasic_get_status(const VMState *vm) {
    return vm->status;
}

static inline void tbasic_set_status(Driver *d, VMStatus status) {
    d->vm.status = status;
}

static inline int tbasic_pop_n(Driver *d, uint8_t n) {
    d->vm.sp -= n;

    return d->vm.sp;
}

static inline int tbasic_push_nil(Driver *d) {
    d->vm.sp++;
    d->vm.stack[d->vm.sp] = make_value_none();

    return d->vm.sp;
}

static inline int tbasic_push_bool(Driver *d, int8_t b) {
    d->vm.sp++;
    d->vm.stack[d->vm.sp] = make_value_bool(b);

    return d->vm.sp;
}

static inline int tbasic_push_int(Driver *d, int i) {
    d->vm.sp++;
    d->vm.stack[d->vm.sp] = make_value_int(i);

    return d->vm.sp;
}

static inline int tbasic_push_float(Driver *d, float f) {
    d->vm.sp++;
    d->vm.stack[d->vm.sp] = make_value_real(f);

    return d->vm.sp;
}

static inline int tbasic_push_string(Driver *d, const char *cstr, size_t n) {
    mystr temp_s;
    mystr_res(&temp_s, DEFAULT_STRING_SIZE);

    if (!mystr_append_raw(&temp_s, cstr, n)) {
        FATAL_ABORT("TBAPI ERROR", __FILE__, __LINE__, "Failed to fill mystr for passed string of const char *cstr, allocation may have failed.");
    }

    String *s_object = alloc_string_of_mystr(&temp_s);

    if (!s_object) {
        FATAL_ABORT("TBAPI ERROR", __FILE__, __LINE__, "Failed to create TBasic string, allocation may have failed.");
    }

    const int16_t s_object_id = heap_store(&d->vm.heap, (ObjMutPtr)s_object);

    d->vm.sp++;
    d->vm.stack[d->vm.sp] = make_value_obj(s_object_id);

    return d->vm.sp;
}

static inline int tbasic_push_value(Driver *d, Value v) {
    d->vm.sp++;
    d->vm.stack[d->vm.sp] = v;

    return d->vm.sp;
}

/**
 * @brief Invokes the TBasic interpreter in a reusable manner, keeping the VMState intact and thus avoiding extra overhead from creating and destroying it.
 * 
 * @param app A `Driver` pre-created from `tbasic_make_driver`.
 * @param pg A `Program` loaded from a byte buffer with `load_bc.h ~ tbl_load_from(TBLoader *ld, const uint8_t *data, size_t n)`.
 * @return Value 
 */
static inline Value tbasic_invoke(Driver *app, const Value *argv, size_t argc) {
    restart_vm(&app->vm);

    for (size_t arg_i = 0; arg_i < argc; arg_i++) {
        tbasic_push_value(app, argv[arg_i]);
    }

    vm_run(&app->vm);

    return vm_result(&app->vm);
}

#endif