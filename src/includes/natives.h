#ifndef TBASIC_NATIVES_H
#define TBASIC_NATIVES_H



#include "mystr.h"
#include "vm.h"



void native_print_str(const mystr *strings, int id);

VMStatus native_print(VMState *s);

VMStatus native_powf(VMState *s);

VMStatus native_sqrtf(VMState *s);

VMStatus native_clampf(VMState *s);

VMStatus native_floorf(VMState *s);

VMStatus native_ceilf(VMState *s);

VMStatus native_console_readln(VMState *s);

VMStatus native_console_reset(VMState *s);

VMStatus native_stoi(VMState *s);

VMStatus native_stof(VMState *s);

VMStatus native_mkiter(VMState *s);

VMStatus native_mviter(VMState *s);

VMStatus native_pkiter(VMState *s);



#endif
