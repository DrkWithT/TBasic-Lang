#include "obj_fs.h"


static const char *flags_to_fopen_mode(int mode) {
    switch (mode) {
        case 0: return "r";
        case 1: return "rb";
        case 2: return "w";
        case 3: return "wb";
        default: return "r";
    }
}

FileStream *alloc_fs(const char *path, int mode) {
    FileStream *temp = ALLOC_TYPE(FileStream);

    if (temp != NULL) {
        FILE *fs = fopen(path, flags_to_fopen_mode(mode));

        temp->base = (ObjBase) {
            .meta = {
                .flags = (mode >= 2 && mode <= 3) ? oflag_mutable : 0x00,
                .tag = otag_fs
            },
            .del = fs_del_fn,
            .as_bool = fs_as_bool_fn,
            .get_v = fs_get_v_fn,
            .set_v = fs_set_v_fn,
            .display = fs_display_fn,
            .invoke = fs_invoke,
            .iterate = fs_iterate_fn,
        };
        temp->f = fs;
        temp->name = path;
    }

    return temp;
}

void fs_del_fn(void *self) {
    FileStream *fs = (FileStream *)self;

    if (fs->f != NULL) {
        fclose(fs->f);
        fs->f = NULL;
    }
}

int8_t fs_as_bool_fn(const void *self) {
    const FileStream *fs = (const FileStream *)self;

    return fs->f != NULL && !feof(fs->f) && !ferror(fs->f);
}

Value fs_get_v_fn(const void *self, Value key) {
    const FileStream *fs = (const FileStream *)self;
    const int query_option = (key.tag == vtag_int) ? key.data.i : -1;

    if (!fs->f) {
        return make_value_none();
    }

    switch (query_option) {
        case 0: return make_value_int(ftell(fs->f));
        case 1: return make_value_bool(ferror(fs->f) != 0);
        case 2: return make_value_bool(feof(fs->f) != 0);
        default: return make_value_none();
    }
}

int8_t fs_set_v_fn(void *self, Value key, Value item) {
    FileStream *fs = (FileStream *)self;
    const int query_option = (key.tag == vtag_int) ? key.data.i : -1;

    if (!fs->f) {
        return 0;
    }

    switch (query_option) {
        case 0:
            if (item.tag == vtag_int) {
                return fseek(fs->f, item.data.i, SEEK_SET) == 0;
            } else {
                return 1; // no-effect
            }
        case 1: case 2: default: return 1; // no effect
    }
}

void fs_display_fn(const void *self, const void *vm_state) {
    const FileStream *fs = (const FileStream *)self;

    if (!fs->f) {
        printf("FileStream(<dead-state>)\n");
    } else {   
        printf("FileStream(path = %s, pos = %i, has_err = %s, at_eof = %s)",
            fs->name,
            (int)ftell(fs->f),
            (ferror(fs->f) != 0) ? "yes" : "no",
            (feof(fs->f) != 0) ? "yes" : "no"
        );
    }
}

uint8_t fs_invoke(void *self, void *vm, const Instruction *caller_ip, const Value *caller_cvp, Value *stack_p, int16_t argc) {
    // ? stub function, NO OP
    return 0;
}

Value fs_iterate_fn(void* self, void *vm) {
    // ! FileStreams aren't iterable since they're not sequences. They can be written to / read from in blobs though.
    return make_value_none();
}
