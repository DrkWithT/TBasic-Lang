#include "basics.h"
#include "objects.h"



void object_base_flag_on(ObjBase *self, ObjFlag flag) {
    self->meta.flags |= flag;
}

void object_base_flag_off(ObjBase *self, ObjFlag flag) {
    self->meta.flags &= (uint8_t)~flag;
}

int8_t object_base_flag_get(const ObjBase *self, ObjFlag flag) {
    switch (flag) {
    case oflag_mutable: return (self->meta.flags & flag);
    case oflag_iterable: return (self->meta.flags & flag) >> 1;
    case oflag_callable: return (self->meta.flags & flag) >> 2;
    default: return 0;
    }
}



void heap_dud(ObjHeap *self) {
    self->cells = calloc(DEFAULT_HEAP_CAPACITY, sizeof(ObjPtr));
    self->free_ids = calloc(DEFAULT_HEAP_CAPACITY, sizeof(int16_t));
    self->cost_to_gc = (DEFAULT_HEAP_CAPACITY * 2 * OBJECT_COST) / 3; // ? 66% --> GC triggers
    self->cost = 0;
    self->next_free_id_pos = -1;
    self->next_cell_id = 0;
    self->tenure_count = 0;
    self->cell_capacity = DEFAULT_HEAP_CAPACITY;
}

void heap_new(ObjHeap *self, int16_t cells) {
    self->cells = calloc(cells, sizeof(ObjPtr));
    self->free_ids = calloc(cells, sizeof(int16_t));
    self->cost_to_gc = (cells * 2 * OBJECT_COST) / 3;
    self->cost = 0;
    self->next_free_id_pos = -1;
    self->next_cell_id = 0;
    self->tenure_count = 0;
    self->cell_capacity = cells;
}

void heap_del(ObjHeap *self) {
    if (self->cells != NULL) {
        for (int16_t cell_i = 0; cell_i < self->cell_capacity; cell_i++) {
            ObjMutPtr target_cell = self->cells[cell_i];

            if (!target_cell) continue;

            if (target_cell->meta.tag == otag_list) {
                target_cell->del(target_cell);
                free(target_cell);
                target_cell = NULL;
            }
        }

        free(self->cells);
        self->cells = NULL;
    }



    if (self->free_ids != NULL) {
        free(self->free_ids);
        self->free_ids = NULL;
    }
}

size_t heap_is_ripe(const ObjHeap *self) {
    return self->cost > self->cost_to_gc;
}

int16_t heap_gen_id(ObjHeap *self) {
    int16_t temp_id = DUD_HEAP_ID;

    // ? If valid, pop-off an ID from the free list... LIFO for easy implementation.
    if (self->next_free_id_pos >= 0) {
        temp_id = self->free_ids[self->next_free_id_pos];
        self->next_free_id_pos--;
    } else if (self->next_cell_id < self->cell_capacity) {
        temp_id = self->next_cell_id;
        self->next_cell_id++;
    }

    return temp_id;
}

void heap_reserve_id(ObjHeap *self, int16_t id) {
    self->next_free_id_pos++;
    self->free_ids[self->next_free_id_pos] = id;
}

void heap_tenure(ObjHeap *self) {
    self->tenure_count = self->next_cell_id;
}

const ObjBase *heap_get(const ObjHeap *self, int16_t id) {
    if (id < 0 || id >= self->next_cell_id) {
        return NULL;
    }

    return self->cells[id];
}

ObjBase *heap_getm(ObjHeap *self, int16_t id) {
    if (id < 0 || id >= self->next_cell_id) {
        return NULL;
    }

    return self->cells[id];
}

int16_t heap_store(ObjHeap *self, ObjMutPtr object) {
    int16_t dest_id = heap_gen_id(self);

    if (dest_id != DUD_HEAP_ID) {   
        self->cells[dest_id] = object;
        self->cost += OBJECT_COST;
    }

    return dest_id;
}

void heap_erase(ObjHeap *self, int16_t cell_id) {
    if (cell_id < self->tenure_count || cell_id >= self->next_cell_id) {
        return;
    }

    ObjMutPtr target = self->cells[cell_id];
    self->cells[cell_id] = NULL;

    target->del(target);
    free(target);
    self->cost -= OBJECT_COST;

    heap_reserve_id(self, cell_id);
}
