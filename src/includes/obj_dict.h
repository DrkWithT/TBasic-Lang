#ifndef TBASIC_DICT_OBJECT_H
#define TBASIC_DICT_OBJECT_H

#include "objects.h"
#include "vm.h"



#define MERSENNE_PRIME_N(n) (ssize_t)(1 + (1 << n))

ssize_t alias_f32_to_ssz(float f);
ssize_t hash_value(const Value *v);

typedef struct ts_struct_prop_t {
    ssize_t hash;
    Value data;
    struct ts_struct_prop_t *l;
    struct ts_struct_prop_t *r;
    uint8_t flags;
} Prop;

typedef const Prop* PropPtr;
typedef Prop* PropMutPtr;

PropMutPtr alloc_prop_node(const Value *key, const Value *item, PropMutPtr left, PropMutPtr right, uint8_t flags);
void ptree_del(PropMutPtr root);
int8_t ptree_empty(PropPtr root);
PropPtr ptree_get(PropPtr root, const Value *key);
PropMutPtr ptree_getm(PropMutPtr root, const Value *key);

// ? If the node is unfound via its hash, a new one is implicitly inserted. Otherwise, an existing one is used. Either case will return a node by mutable pointer.
PropMutPtr ptree_set(PropMutPtr *root, const Value *key, const Value *data, uint8_t flags);



typedef struct ts_struct_obj_t {
    ObjBase base;
    Prop *root;
} Dict;

Dict *alloc_dict();

void dict_del_fn(void *self);
int8_t dict_as_bool_fn(const void *self);
Value dict_get_v_fn(const void *self, Value key);
int8_t dict_set_v_fn(void *self, Value key, Value item);
void dict_display_fn(const void *self, const void *vm);

// ? stub function: NOOP
uint8_t dict_invoke(void *self, void *vm, Instruction *caller_ip, const Value *caller_cvp, Value *stack_p, int16_t argc);

#endif