#ifndef TBASIC_GENERAL_STDLIB_H
#define TBASIC_GENERAL_STDLIB_H

#include "vm.h"



VMStatus native_powf(VMState *s);

VMStatus native_sqrtf(VMState *s);

VMStatus native_clampf(VMState *s);

VMStatus native_floorf(VMState *s);

VMStatus native_ceilf(VMState *s);

VMStatus native_stoi(VMState *s);

VMStatus native_stof(VMState *s);

#endif