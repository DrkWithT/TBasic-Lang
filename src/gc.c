#include <string.h>
#include <stdio.h>

#include "basics.h"
#include "obj_list.h"
#include "obj_dict.h"
#include "obj_closure.h"
#include "gc.h"



void BitSet_new(BitSet *self, uint16_t capacity) {
    const size_t set_bytes_n = (capacity / 8) + 1;
    uint8_t *temp_bytes = calloc(set_bytes_n, sizeof(typeof(self->bits[0])));

    if (temp_bytes != NULL) {
        self->bits = temp_bytes;
        self->capacity = set_bytes_n * BITSET_U8_BITS;
    } else {
        FATAL_ABORT("Alloc Error", __FILE__, __LINE__, "Failed to allocate BitSet::bits.")
    }
}

void BitSet_del(BitSet *self) {
    if (self->bits != NULL) {
        free(self->bits);
        self->bits = NULL;
    }
}

uint16_t BitSet_len(const BitSet *self) {
    return self->capacity;
}

int8_t BitSet_get_at(const BitSet *self, uint16_t pos) {
    const uint16_t byte_pos = pos / 8;
    const uint8_t bit_pos = pos & 0b00000111;

    return (self->bits[byte_pos] & (1 << bit_pos)) >> bit_pos;
}

void BitSet_set_at(BitSet *self, uint16_t pos) {
    const uint16_t byte_pos = pos / 8;
    const uint8_t bit_pos = pos & 0b00000111;

    self->bits[byte_pos] |= (1 << bit_pos);
}

void BitSet_clear(BitSet *self) {
    memset(self->bits, 0, (self->capacity / BITSET_U8_BITS));
}



void IdQueue_new(IdQueue *self, int16_t capacity) {
    int16_t *temp_buf = calloc(capacity, sizeof(typeof(self->items[0])));

    if (temp_buf != NULL) {
        for (uint16_t i = 0; i < capacity; i++) {
            temp_buf[i] = DUD_HEAP_ID;
        }

        self->items = temp_buf;
        self->head_pos = 0;
        self->tail_pos = 0;
        self->length = 0;
        self->capacity = capacity;
    } else {
        FATAL_ABORT("Alloc Error", __FILE__, __LINE__, "Failed to allocate IdQueue::items.")
    }
}

void IdQueue_del(IdQueue *self) {
    if (self->items != NULL) {
        free(self->items);
        self->items = NULL;
    }
}

int8_t IdQueue_exhausted(const IdQueue *self) {
    return self->head_pos == self->tail_pos;
}

int8_t IdQueue_push(IdQueue *self, int16_t id) {
    self->tail_pos = (self->tail_pos + 1) % self->capacity;
    self->items[self->tail_pos] = id;

    return !IdQueue_exhausted(self);
}

int16_t IdQueue_pop(IdQueue *self) {
    if (IdQueue_exhausted(self)) {
        return DUD_HEAP_ID;
    }

    const int16_t temp_id = self->items[self->head_pos];
    self->head_pos = (self->head_pos + 1) % self->capacity;

    return temp_id;
}

void IdQueue_reset(IdQueue *self) {
    for (uint16_t i = 0; i < self->capacity; i++) {
        self->items[i] = DUD_HEAP_ID;
    }

    self->head_pos = 0;
    self->tail_pos = 0;
}



void PtrQueue_new(PtrQueue *self, int16_t capacity) {
    const void **temp_buf = calloc(capacity, sizeof(typeof(self->ptrs[0])));

    if (temp_buf != NULL) {
        for (uint16_t i = 0; i < capacity; i++) {
            temp_buf[i] = NULL;
        }

        self->ptrs = temp_buf;
        self->head_pos = 0;
        self->tail_pos = 0;
        self->length = 0;
        self->capacity = capacity;
    } else {
        FATAL_ABORT("Alloc Error", __FILE__, __LINE__, "Failed to allocate PtrQueue::items.")
    }
}

void PtrQueue_del(PtrQueue *self) {
    if (self->ptrs != NULL) {
        free(self->ptrs);
        self->ptrs = NULL;
    }
}

int8_t PtrQueue_exhausted(const PtrQueue *self) {
    return self->head_pos == self->tail_pos;
}

int8_t PtrQueue_push(PtrQueue *self, const void *p) {
    self->tail_pos = (self->tail_pos + 1) % self->capacity;
    self->ptrs[self->tail_pos] = p;

    return !PtrQueue_exhausted(self);
}

const void *PtrQueue_pop(PtrQueue *self) {
    if (PtrQueue_exhausted(self)) {
        return NULL;
    }

    const void *temp_ptr = self->ptrs[self->head_pos];
    self->head_pos = (self->head_pos + 1) % self->capacity;

    return temp_ptr;
}

void PtrQueue_reset(PtrQueue *self) {
    for (uint16_t i = 0; i < self->capacity; i++) {
        self->ptrs[i] = NULL;
    }

    self->head_pos = 0;
    self->tail_pos = 0;
}



// ? NOTE: max_object_ids must equal the heap object capacity.
void GCState_new(GCState *self, int16_t max_object_ids) {
    BitSet_new(&self->reach_bits, (uint16_t)max_object_ids);
    IdQueue_new(&self->next_ids, max_object_ids);
}

void GCState_del(GCState *self) {
    BitSet_del(&self->reach_bits);
    IdQueue_del(&self->next_ids);
}

static void GCState_mark_dict_nodes(GCState *self, const Dict *dict) {
    PtrQueue node_queue;
    PtrQueue_new(&node_queue, DEFAULT_HEAP_CAPACITY);

    PtrQueue_push(&node_queue, dict->root);

    while (!PtrQueue_exhausted(&node_queue)) {
        PropPtr next_prop = PtrQueue_pop(&node_queue);

        if (next_prop == NULL) {
            continue;
        }

        if (next_prop->data.tag == vtag_obj_id) {
            BitSet_set_at(&self->reach_bits, next_prop->data.data.obj_id);
        }

        if (next_prop->l) {
            // ! Whenever the ring buffer of node pointers is prematurely full, skip ahead to process what we can... Although this handles attempted overwrites of queued pointers, the GC could be swamped on heavy workloads (which TBasic is not meant for) and collect "unreached" but used objects in a swept dict.
            if (!PtrQueue_push(&node_queue, next_prop->l)) {
                continue;
            }
        }

        if (next_prop->r) {
            // ! See previous note ~ line 200.
            if (!PtrQueue_push(&node_queue, next_prop->r)) {
                continue;
            }
        }
    }

    PtrQueue_del(&node_queue);
}

static void GCState_mark_upvals(GCState *self, const Value *upvals_p, uint32_t n) {
    for (int16_t i = 0; i < n; i++) {
        if (upvals_p[i].tag == vtag_obj_id) {
            BitSet_set_at(&self->reach_bits, (upvals_p + i)->data.obj_id);
        }
    }
}

void GCState_collect(GCState *self, ObjHeap *heap, const Value *stack_ptr, int stack_sp) {
    if (!heap_is_ripe(heap)) {
        return;
    }

    // TODO 1: queue initials via populating bitset
    for (int stack_offset = 0; stack_offset < stack_sp; stack_offset++) {
        if (stack_ptr[stack_sp].tag == vtag_obj_id) {
            const int16_t temp_obj_id = stack_ptr[stack_sp].data.obj_id;

            if (!IdQueue_push(&self->next_ids, temp_obj_id)) {
                break;
            }
        }
    }

    // TODO 2: BFS with queue to sweep object ID space for those not in bitset... mark reached ones...
    while (!IdQueue_exhausted(&self->next_ids)) {
        const int16_t temp_obj_id = IdQueue_pop(&self->next_ids);

        if (temp_obj_id == DUD_HEAP_ID) {
            continue;
        }

        ObjPtr temp_obj = heap_get(heap, temp_obj_id);

        if (!temp_obj) {
            continue;
        } else if (temp_obj->meta.tag == otag_list) {
            BitSet_set_at(&self->reach_bits, temp_obj_id);
            const List *list_obj = (const List *)temp_obj;

            for (size_t item_i = 0; item_i < list_obj->data.length; item_i++) {
                const Value *item_ref = AnyVec_Value_get(&list_obj->data, item_i);

                if (item_ref->tag == vtag_obj_id) {
                    IdQueue_push(&self->next_ids, item_ref->data.obj_id);
                }
            }
        } else if (temp_obj->meta.tag == otag_string) {
            BitSet_set_at(&self->reach_bits, temp_obj_id);
        } else if (temp_obj->meta.tag == otag_dict) {
            GCState_mark_dict_nodes(self, (const Dict *)temp_obj);
        } else if (temp_obj->meta.tag == otag_closure) {
            const Closure *closure = (const Closure *)temp_obj;
            GCState_mark_upvals(self, closure->upvals, closure->count);
        }
    }

    // TODO 3: free unreached ones...
    const uint16_t slot_bits_n = BitSet_len(&self->reach_bits);
    int16_t count = 0;

    for (uint16_t slot_bit_id = 0; slot_bit_id < slot_bits_n; slot_bit_id++) {
        if (!BitSet_get_at(&self->reach_bits, slot_bit_id)) {
            heap_erase(heap, slot_bit_id);
            count++;
        }
    }

    BitSet_clear(&self->reach_bits);
    IdQueue_reset(&self->next_ids);

    #if TBASIC_DEBUG_GC_STATE
        printf("GC: Collected %d objects.\n", count);
    #endif
}
