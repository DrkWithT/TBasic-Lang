#ifndef TBASIC_IO_STDLIB_H
#define TBASIC_IO_STDLIB_H



#include "mystr.h"
#include "vm.h"



void native_print_str(const mystr *strings, int id);

VMStatus native_print(VMState *s);

VMStatus native_console_readln(VMState *s);

VMStatus native_console_reset(VMState *s);

VMStatus native_fopen(VMState *s);

VMStatus native_fclose(VMState *s);

VMStatus native_fgetc(VMState *s);

VMStatus native_fputc(VMState *s);

VMStatus native_fread(VMState *s);

VMStatus native_fwrite(VMState *s);

#endif
