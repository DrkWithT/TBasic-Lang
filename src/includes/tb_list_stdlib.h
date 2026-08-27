#ifndef TBASIC_LIST_STDLIB_H
#define TBASIC_LIST_STDLIB_H

#include "vm.h"



VMStatus native_lsrev(VMState *s);

VMStatus native_lscat(VMState *s);

VMStatus native_lsclr(VMState *s);

VMStatus native_lscut(VMState *s);

#endif