#ifndef TBASIC_OBJECT_STDLIB_H
#define TBASIC_OBJECT_STDLIB_H

#include "vm.h"



VMStatus native_mkiter(VMState *s);

VMStatus native_mviter(VMState *s);

VMStatus native_pkiter(VMState *s);

VMStatus native_thaw(VMState *s);

VMStatus native_freeze(VMState *s);

#endif