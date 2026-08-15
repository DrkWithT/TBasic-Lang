#include <stdlib.h>
#include "obj_dict.h"



static const ssize_t vhash_places[] = {
    1,
    5,
    25,
    125
};

ssize_t alias_f32_to_ssz(float f) {
    int tmp = 0;

    memcpy(&tmp, &f, sizeof(int));

    return tmp;
}

ssize_t hash_value(const Value *v) {
    int32_t vscalar = 0;

    switch (v->tag) {
    case vtag_nil:
        vscalar = 0;
        break;
    case vtag_bool:
        vscalar = 1 + (v->data.byte << 1);
        break;
    case vtag_int:
        vscalar = v->data.i;
        break;
    case vtag_real:
        vscalar = alias_f32_to_ssz(v->data.f);
        break;
    case vtag_strid:
        vscalar = v->data.i;
        break;
    case vtag_obj_id:
    default:    
        vscalar = v->data.obj_id;
        break;
    }

    const uint8_t *vs_bytes = (const uint8_t *)(&vscalar);

    return (vs_bytes[0] * vhash_places[0])
        + (vs_bytes[1] * vhash_places[1])
        + (vs_bytes[2] * vhash_places[2])
        + (vs_bytes[3] * vhash_places[3]);
}

PropMutPtr alloc_prop_node(const Value *key, const Value *item, PropMutPtr left, PropMutPtr right, uint8_t flags) {
    PropMutPtr temp = ALLOC_TYPE(Prop);

    if (temp != NULL) {
        temp->hash = hash_value(key);
        temp->data = *item;
        temp->l = left;
        temp->r = right;
        temp->flags = flags;
    }

    return temp;
}

void ptree_del(PropMutPtr root) {
    if (root == NULL) {
        return;
    }

    ptree_del(root->l);
    ptree_del(root->r);
    free(root);
}

int8_t ptree_empty(PropPtr root) {
    return root == NULL;
}

PropPtr ptree_get(PropPtr root, const Value *key) {
    const ssize_t key_h = hash_value(key);
    PropPtr temp = root;

    while (temp != NULL) {
        const ssize_t temp_h = temp->hash;

        if (key_h < temp_h) {
            temp = temp->l;
        } else if (key_h > temp_h) {
            temp = temp->r;
        } else {
            return temp;
        }
    }

    return NULL;
}

PropMutPtr ptree_getm(PropMutPtr root, const Value *key) {
    const ssize_t key_h = hash_value(key);
    PropMutPtr temp = root;

    while (temp != NULL) {
        const ssize_t temp_h = temp->hash;

        if (key_h < temp_h) {
            temp = temp->l;
        } else if (key_h > temp_h) {
            temp = temp->r;
        } else {
            return temp;
        }
    }

    return NULL;
}

PropMutPtr ptree_set(PropMutPtr *root, const Value *key, const Value *data, uint8_t flags) {
    const ssize_t key_h = hash_value(key);
    PropMutPtr prev = NULL;
    PropMutPtr curr = *root;

    if (curr == NULL) {
        *root = alloc_prop_node(key, data, NULL, NULL, flags);
        return *root;
    }

    int8_t curr_was_left = 0;

    while (curr != NULL) {
        const ssize_t curr_h = curr->hash;

        if (key_h < curr_h) {
            prev = curr;
            curr = curr->l;
            curr_was_left = 1;
        } else if (key_h > curr_h) {
            prev = curr;
            curr = curr->r;
            curr_was_left = 0;
        } else {
            curr->data = *data;
            curr->flags = flags;
            return curr;
        }
    }

    if (curr_was_left) {
        prev->l = alloc_prop_node(key, data, NULL, NULL, flags);
        curr = prev->l;
    } else {
        prev->r = alloc_prop_node(key, data, NULL, NULL, flags);
        curr = prev->r;
    }

    return curr;
}



Dict *alloc_dict() {
    Dict *temp = ALLOC_TYPE(Dict);

    if (temp != NULL) {
        temp->base = (ObjBase) {
            .meta = {
                .tag = otag_dict,
                .flags = oflag_mutable
            },
            .del = dict_del_fn,
            .as_bool = dict_as_bool_fn,
            .get_v = dict_get_v_fn,
            .set_v = dict_set_v_fn,
            .display = dict_display_fn,
            .invoke = dict_invoke
        };

        temp->root = NULL;
    }

    return temp;
}

void dict_del_fn(void *self) {
    Dict *self_as_dict = (Dict *)self;

    ptree_del(self_as_dict->root);
}

int8_t dict_as_bool_fn(const void *self) {
    const Dict *self_as_dict = (const Dict *)self;

    return !ptree_empty(self_as_dict->root);
}

Value dict_get_v_fn(const void *self, Value key) {
    const Dict *self_as_dict = (const Dict *)self;

    PropPtr result = ptree_get(self_as_dict->root, &key);

    return (result != NULL) ? result->data : make_value_none();
}

int8_t dict_set_v_fn(void *self, Value key, Value item) {
    Dict *self_as_dict = (Dict *)self;

    (void) ptree_set(&self_as_dict->root, &key, &item, oflag_mutable);

    return 1;
}

void dict_display_fn(const void *self, MAYBE_UNUSED const void *vm) {
    const Dict *self_as_dict = (const Dict *)self;
    printf("Dict(address = %p, data = ...)", self_as_dict);
}

uint8_t dict_invoke(void *self, void *vm, const Instruction *caller_ip, const Value *caller_cvp, Value *stack_p, int16_t argc) {
    return 0;
}
