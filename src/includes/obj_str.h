#ifndef TBASIC_STRING_OBJECT_H
#define TBASIC_STRING_OBJECT_H



#include "mystr.h"
#include "objects.h"
#include "vm.h"



typedef struct ts_string_t String;
typedef const String* StrPtr;
typedef String* StrMutPtr;

typedef struct ts_string_t {
    ObjBase base;
    mystr data;
} String;

String *alloc_string(size_t capacity);
String *alloc_string_of_mystr(mystr *s);

void string_del_fn(void *self);
int8_t string_as_bool_fn(const void *self);
Value string_get_v_fn(const void *self, Value key);
int8_t string_set_v_fn(void *self, Value key, Value item);
void string_display_fn(const void *self, const void *vm_state);

// ? stub function: NOOP
uint8_t string_invoke(void *self, void *vm, Instruction *caller_ip, const Value *caller_cvp, Value *stack_p, int16_t argc);

#endif
