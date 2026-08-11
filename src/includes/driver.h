#ifndef TBASIC_DRIVER_H
#define TBASIC_DRIVER_H

#include <stdio.h>

#include "vec.h"
#include "mystr.h"
#include "compiler.h"
#include "optimizer.h"
#include "vm.h"



#define CONFIG_DEFAULT_VM_LOCALS 16384
#define CONFIG_DEFAULT_VM_RECUR_LIMIT 64
#define CONFIG_DEFAULT_VM_HEAP_POPULATION 256
#define TBASIC_VERSION_MAJOR 0
#define TBASIC_VERSION_MINOR 11
#define TBASIC_VERSION_PATCH 0

mystr read_file(const char *fname);

typedef struct driver_config_t {
    const char *title;
    int stack_capacity;
    int16_t heap_capacity;
    uint8_t recursion_max;
    uint8_t version_major;
    uint8_t version_minor;
    uint8_t version_patch;
} DriverConfig;

typedef enum driver_flag_t : uint8_t {
    dflag_info,
    dflag_dis_bc,
    dflag_load_bc,
    dflag_run_bc,
    dflag_invalid_opts,
    dflag_last,
} DriverFlag;

STUB_SCALAR_VEC(NativeFn)



typedef struct ts_driver_t {
    Compiler compiler;
    VMState vm;
    Optimizer optimizer;
    ScalarVec_NativeFn natives;
    DriverConfig config;
    int8_t flags[dflag_last];
} Driver;

// ! WARNING: This function doesn't initialize `VMState vm` because it requires native functions, stack & heap sizing, etc. to be resolved later. Note that `tbasic_make_driver` from `tb_api.h` can be used for a reusable driver to use with `tbasic_invoke`.
void driver_dud(Driver *d, const DriverConfig *config);
void driver_del(Driver *d);

static inline int8_t driver_get_flag(const Driver *d, DriverFlag flag) {
    return d->flags[flag];
}

static inline void driver_set_flag(Driver *d, DriverFlag flag, int8_t arg) {
    d->flags[flag] = arg;
}

void driver_bind_native(Driver *d, charspan name, NativeFn fn);
Program driver_compile(Driver *d, const char *file_path);
int driver_run(Driver *d, const char *file_path);

#endif