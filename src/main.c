#include <stdio.h>
#include "tb_gen_stdlib.h"
#include "tb_obj_stdlib.h"
#include "tb_list_stdlib.h"
#include "tb_io_stdlib.h"
#include "tb_api.h"

/**
 * @brief Array of corresponding names of stdlib routines.
 */
const charspan builtin_names[] = {
    (charspan) {.data = "powf", .length = 4},
    (charspan) {.data = "sqrtf", .length = 5},
    (charspan) {.data = "clampf", .length = 6},
    (charspan) {.data = "floorf", .length = 6},
    (charspan) {.data = "ceilf", .length = 5},
    (charspan) {.data = "stoi", .length = 4},
    (charspan) {.data = "stof", .length = 4},
    (charspan) {.data = "mkiter", .length = 6},
    (charspan) {.data = "mviter", .length = 6},
    (charspan) {.data = "pkiter", .length = 6},
    (charspan) {.data = "thaw", .length = 4},
    (charspan) {.data = "freeze", .length = 6},
    // todo: impl. and add dict utils...
    (charspan) {.data = "lsrev", .length = 5},
    (charspan) {.data = "lscat", .length = 5},
    (charspan) {.data = "lsclr", .length = 5},
    (charspan) {.data = "lscut", .length = 5},
    (charspan) {.data = "creadln", .length = 7},
    (charspan) {.data = "creset", .length = 6},
    (charspan) {.data = "print", .length = 5},
};

/**
 * @brief Array of loadable pointers to TBasic stdlib routines.
 * Contents:
 * - Math
 * - Object Utils for strings, iterators, lists, and dictionaries.
 * - Console I/O
 * - File I/O
 */
const NativeFn builtin_funcs[] = {
    native_powf,
    native_sqrtf,
    native_clampf,
    native_floorf,
    native_ceilf,
    native_stoi,
    native_stof,
    native_mkiter,
    native_mviter,
    native_pkiter,
    native_thaw,
    native_freeze,
    // todo: impl. and add dict utils...
    native_lsrev,
    native_lscat,
    native_lsclr,
    native_lscut,
    native_console_readln,
    native_console_reset,
    native_print,
};

int main(int argc, const char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Too few arguments. Try ./tbasic -i for information.\n");
        return 1;
    }

    const size_t preload_count = sizeof(builtin_funcs) / sizeof(NativeFn);
    Driver tbasic_app = tbasic_make_driver(builtin_names, builtin_funcs, preload_count);

    const int main_status = driver_run(&tbasic_app, argv, argc);

    driver_del(&tbasic_app);
}
