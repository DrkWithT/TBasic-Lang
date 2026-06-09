#include <stdio.h>

#include "mystr.h"
#include "natives.h"
#include "driver.h"



static const char *project_name =                            
" _____ _____         _     \n"
"|_   _| __  |___ ___|_|___ \n"
"  | | | __ -| .'|_ -| |  _|\n"
"  |_| |_____|__,|___|_|___|\n";
                           

int main(int argc, char *argv[]) {
    const char *source_fpath = NULL;
    int8_t show_info = 0;
    int8_t dump_bc = 0;
    int8_t allow_run = 0;

    if (argc < 2) {
        fprintf(stderr, "Too few arguments. Try ./tbasic -i for information.\n");
        return 1;
    }

    for (int16_t arg_i = 0; arg_i < argc - 1; arg_i++) {
        if (!strcmp(argv[1 + arg_i], "-i")) { show_info = 1;}
        else if (!strcmp(argv[1 + arg_i], "-d")) { dump_bc = 1; }
        else if (!strcmp(argv[1 + arg_i], "-r")) { allow_run = 1; }
        else { source_fpath = argv[1 + arg_i]; }
    }

    DriverConfig config = {
        .title = project_name,
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

    return driver_run(&app, source_fpath);
}
