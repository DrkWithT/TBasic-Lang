#ifndef TBASIC_OBJECTS_H
#define TBASIC_OBJECTS_H

#include "value.h"

#define DEFAULT_HEAP_CAPACITY ((int16_t) 512)
#define DUD_HEAP_ID ((int16_t) -1)
#define OBJECT_COST ((size_t) 64)

#define DEFAULT_STRING_POOL_CAPACITY ((int16_t) 512)
#define DEFAULT_STRING_SIZE ((size_t) 8)
#define STRING_PREPEND_KEY ((int) -1)
#define STRING_COST OBJECT_COST



// ? Forward declare Instruction for closure use.
typedef struct code_vec_t Instruction;

typedef enum obj_tag_t : uint8_t {
    otag_dud,
    otag_list,
    otag_string,
    otag_dict,
    otag_err,
    otag_closure,
} ObjTag;

typedef enum obj_flags_t : uint8_t {
    oflag_mutable = 0b00000001,
    oflag_iterable = 0b00000010,
    oflag_callable = 0b00000100,
} ObjFlag;

// ? This stores type_info to discern how to downcast ObjBase --> List, etc.
typedef struct obj_head_t {
    ObjTag tag;         // ? enclosed object's discriminator
    uint8_t flags;      // ? object's metadata / flags
} ObjHead;

// ? Stores a tiny interface of objects: destruct, test truthiness, get / set values... all in a shoddy vtable.
typedef struct obj_base_t {
    ObjHead meta;
    void (*del) (void *self); // ! This may seem to "leak" objects in list for example, but the GC will catch these anyhow.
    int8_t (*as_bool) (const void *self);
    Value (*get_v) (const void *self, Value key);
    int8_t (*set_v) (void *self, Value key, Value item);
    void (*display) (const void *self, const void *vm);
    uint8_t (*invoke) (void *self, void *vm, const Instruction *caller_ip, const Value *caller_cvp, Value *stack_p, int16_t argc); // todo
} ObjBase;

typedef const ObjBase* ObjPtr;
typedef ObjBase* ObjMutPtr;

void object_base_flag_on(ObjBase *self, ObjFlag flag);
void object_base_flag_off(ObjBase *self, ObjFlag flag);
int8_t object_base_flag_get(const ObjBase *self, ObjFlag flag);

typedef struct obj_heap_t {
    ObjMutPtr *cells;          // ? preallocated buffer of polymorphic ObjBase ptrs.
    int16_t *free_ids;      // ? preallocated free ID pool
    size_t cost_to_gc;      // ? object overhead required for GC cycle
    size_t cost;            // ? object "overhead" total
    int16_t next_free_id_pos;
    int16_t next_cell_id;
    int16_t tenure_count;
    int16_t cell_capacity;
} ObjHeap;

void heap_dud(ObjHeap *self);
void heap_new(ObjHeap *self, int16_t cells);
void heap_del(ObjHeap *self);

size_t heap_is_ripe(const ObjHeap *self);

int16_t heap_gen_id(ObjHeap *self);
void heap_reserve_id(ObjHeap *self, int16_t id);
void heap_tenure(ObjHeap *self);

const ObjBase *heap_get(const ObjHeap *self, int16_t id);
ObjBase *heap_getm(ObjHeap *self, int16_t id);
int16_t heap_store(ObjHeap *self, ObjMutPtr object);
void heap_erase(ObjHeap *self, int16_t cell_id);

#endif
