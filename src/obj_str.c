#include "obj_str.h"
#include "vm.h"



String *alloc_string(size_t capacity) {
    String *temp = ALLOC_TYPE(String);

    temp->base = (ObjBase) {
        .meta = {
            .tag = otag_string,
            .flags = oflag_mutable,
        },
        .del = string_del_fn,
        .as_bool = string_as_bool_fn,
        .get_v = string_get_v_fn,
        .set_v = string_set_v_fn,
        .display = string_display_fn,
        .invoke = string_invoke
    };

    mystr contents;
    mystr_new(&contents, "");
    temp->data = contents;

    return temp;
}

String *alloc_string_of_mystr(mystr *s) {
    String *temp = ALLOC_TYPE(String);

    temp->base = (ObjBase) {
        .meta = {
            .tag = otag_string,
            .flags = oflag_mutable,
        },
        .del = string_del_fn,
        .as_bool = string_as_bool_fn,
        .get_v = string_get_v_fn,
        .set_v = string_set_v_fn,
        .display = string_display_fn,
        .invoke = string_invoke
    };
    temp->data = *s;

    return temp;
}

void string_del_fn(void *self) {
    StrMutPtr self_as_string = (StrMutPtr)self;

    mystr_del(&self_as_string->data);
}

int8_t string_as_bool_fn(const void *self) {
    StrMutPtr self_as_string = (StrMutPtr)self;

    return !mystr_empty(&self_as_string->data);
}

Value string_get_v_fn(const void *self, Value key) {
    StrMutPtr self_as_string = (StrMutPtr)self;
    const int char_index = (key.tag == vtag_int) ? key.data.i : -1, last_index = self_as_string->data.length;

    if (char_index < 0 || char_index >= last_index) {
        return make_value_none();
    }

    const int ascii_code = self_as_string->data.data[char_index];

    return make_value_int(ascii_code);
}

int8_t string_set_v_fn(void *self, Value key, Value item) {
    StrMutPtr self_as_string = (StrMutPtr)self;

    const int char_index = (key.tag == vtag_int) ? key.data.i : -1, last_index = self_as_string->data.length;
    const char temp_char = (item.tag == vtag_int) ? (char)(item.data.i) : '\0';

    if (char_index >= 0 && char_index < last_index) {
        self_as_string->data.data[char_index] = temp_char; 
        return 1;
    } else if (key.tag == vtag_nil) {
        return mystr_append_raw(&self_as_string->data, &temp_char, 1);
    }

    return 0;
}

void string_display_fn(const void *self, const void *vm_state) {
    StrPtr self_as_string = (StrPtr)self;
    const VMState *vm = (const VMState *)vm_state;

    fwrite(
        mystr_raw(&self_as_string->data),
        sizeof(char),
        mystr_len(&self_as_string->data),
        stdout
    );
    fprintf(stdout, "\n");
}

uint8_t string_invoke(void *self, void *vm, Instruction *caller_ip, const Value *caller_cvp, Value *stack_p, int16_t argc) {
    return 0;
}
