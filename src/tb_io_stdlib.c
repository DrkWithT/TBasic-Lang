#include <stdio.h>
#include "tb_io_stdlib.h"
#include "obj_list.h"
#include "obj_fs.h"



void native_print_str(const mystr *strings, int id) {
    if (id < 0) {
        printf("'...'");
    } else {
        printf("%s", strings[id].data);
    }
}

/*
 * Invariants: 
 * 1. Returns NONE in STACK[CALLEE_BP].
 * 2. The convention is followed for the VM:
 *      CALLEE_BP = SP - ARGC
 *      LOCALS[N] = STACK[CALLEE_BP + 1 + N]
 * Stack Layout: of print(1, 2, 3)
 * | Value(Int(3)) | <--- SP <--- CALLEE_BP + 3
 * | Value(Int(2)) |
 * | Value(Int(1)) | <--- LOCAL_1 <--- CALLEE_BP + 1
 * | Value(Fun-ID) | <--- CALLEE_BP = SP - ARGC = SP - 3 <--- PUT "none"
 */
VMStatus native_print(VMState *s) {
    const int callee_bp = s->bp;
    const int argc = s->sp - s->bp;

    for (int i = 1; i <= argc; i++) {
        const Value *arg_ref = s->stack + callee_bp + i;

        print_value(arg_ref, s);
        printf(" ");
    }

    printf("\n");

    s->sp++;
    s->stack[s->sp] = make_value_none();

    return vm_status_pending;
}

VMStatus native_console_readln(VMState *s) {
    const int callee_bp = s->bp;

    if (feof(stdin)) {
        clearerr(stdin);
    } else if (ferror(stdin)) {
        fprintf(stderr, "STDIN is in an errorneous state, try console_reset().\n");
        return 0;
    }

    mystr input_str;
    mystr_new(&input_str, "");
    char c = '\0';
    size_t rc = 0;

    while (1) {
        rc = fread(&c, sizeof(char), 1, stdin);

        if (c == '\n' || rc <= 0 || feof(stdin)) {
            break;
        } else if (ferror(stdin)) {
            perror("Failed to read line.");
            break;
        } else {
            mystr_append_raw(&input_str, &c, 1);
        }
    }

    s->sp++;
    s->stack[s->sp] = make_value_obj(vm_put_heap_string(s, &input_str));

    return 1;
}

VMStatus native_console_reset(VMState *s) {
    const int callee_bp = s->bp;

    clearerr(stdin);

    s->sp++;
    s->stack[s->sp] = make_value_none();

    return 1;
}

VMStatus native_fopen(VMState *s) {
    const int callee_bp = s->bp;
    const Value path_arg = s->stack[callee_bp + 1];
    const Value mode_arg = s->stack[callee_bp + 2];

    if (path_arg.tag != vtag_strid || mode_arg.tag != vtag_int) {
        s->sp++;
        s->stack[s->sp] = make_value_none();

        return vm_status_pending;
    }

    const mystr *path_str = AnyVec_mystr_get(&s->prgm->strings, path_arg.data.i);
    const int mode_code = mode_arg.data.i;

    ObjMutPtr fs_object_ptr = (ObjMutPtr)alloc_fs(mystr_raw(path_str), mode_code);

    if (!fs_object_ptr) {
        s->sp++;
        s->stack[s->sp] = make_value_none();

        return vm_status_pending;
    }

    const int16_t fs_heap_id = heap_store(&s->heap, fs_object_ptr);

    if (fs_heap_id == DUD_HEAP_ID) {
        fs_object_ptr->del(fs_object_ptr);
        free(fs_object_ptr);

        s->sp++;
        s->stack[s->sp] = make_value_none();

        return vm_status_pending;
    }

    s->sp++;
    s->stack[s->sp] = make_value_obj(fs_heap_id);

    return vm_status_pending;
}

VMStatus native_fclose(VMState *s) {
    const int callee_bp = s->bp;
    const Value fs_arg = s->stack[callee_bp + 1];

    if (fs_arg.tag != vtag_obj_id) {
        s->sp++;
        s->stack[s->sp] = make_value_bool(0);

        return vm_status_pending;
    }

    ObjMutPtr fs_object_ptr = heap_getm(&s->heap, fs_arg.data.obj_id);

    if (!fs_object_ptr || fs_object_ptr->meta.tag != otag_fs) {
        s->sp++;
        s->stack[s->sp] = make_value_bool(0);

        return vm_status_pending;
    }

    // ? Close this FileStream's internal `FILE *f`.
    fs_object_ptr->del(fs_object_ptr);

    s->sp++;
    s->stack[s->sp] = make_value_bool(1);

    return vm_status_pending;
}

VMStatus native_fgetc(VMState *s) {
    const int callee_bp = s->bp;
    const Value fs_arg = s->stack[callee_bp + 1];

    if (fs_arg.tag != vtag_obj_id) {
        s->sp++;
        s->stack[s->sp] = make_value_bool(0);

        return vm_status_pending;
    }

    ObjPtr fs_object_ptr = heap_get(&s->heap, fs_arg.data.obj_id);

    if (!fs_object_ptr || fs_object_ptr->meta.tag != otag_fs) {
        s->sp++;
        s->stack[s->sp] = make_value_bool(0);

        return vm_status_pending;
    }

    const FileStream *fs = (const FileStream *)fs_object_ptr;

    s->sp++;

    if (fs->f != NULL) {
        s->stack[s->sp] = make_value_int(fgetc(fs->f));
    } else {
        s->stack[s->sp] = make_value_none();
    }

    return vm_status_pending;
}

VMStatus native_fputc(VMState *s) {
    const int callee_bp = s->bp;
    const Value fs_arg = s->stack[callee_bp + 1];
    const Value ascii_arg = s->stack[callee_bp + 2];

    if (fs_arg.tag != vtag_obj_id || ascii_arg.tag != vtag_int) {
        s->sp++;
        s->stack[s->sp] = make_value_bool(0);

        return vm_status_pending;
    }

    ObjPtr fs_object_ptr = heap_get(&s->heap, fs_arg.data.obj_id);

    if (!fs_object_ptr || fs_object_ptr->meta.tag != otag_fs) {
        s->sp++;
        s->stack[s->sp] = make_value_bool(0);

        return vm_status_pending;
    }

    const FileStream *fs = (const FileStream *)fs_object_ptr;
    // ? Clamp to unsigned byte value range!
    const int ascii_code = ascii_arg.data.i & 0xff;

    s->sp++;

    if (fs->f != NULL) {
        s->stack[s->sp] = make_value_bool(fputc(ascii_code, fs->f) == 0);
    } else {
        s->stack[s->sp] = make_value_bool(0);
    }

    return vm_status_pending;
}

VMStatus native_fread(VMState *s) {
    const int callee_bp = s->bp;
    const Value fs_arg = s->stack[callee_bp + 1];
    const Value dest_arg = s->stack[callee_bp + 2];
    const Value rc_arg = s->stack[callee_bp + 2];

    if (fs_arg.tag != vtag_obj_id || dest_arg.tag != vtag_obj_id || rc_arg.tag != vtag_int) {
        s->sp++;
        s->stack[s->sp] = make_value_int(-1);

        return vm_status_pending;
    }

    ObjPtr fs_object_ptr = heap_get(&s->heap, fs_arg.data.obj_id);
    ObjMutPtr dest_buf_ptr = heap_getm(&s->heap, dest_arg.data.obj_id);
    int rc = rc_arg.data.i;

    if (!fs_object_ptr || fs_object_ptr->meta.tag != otag_fs
        || !dest_buf_ptr || dest_buf_ptr->meta.tag != otag_list
        || rc < 0) {
        s->sp++;
        s->stack[s->sp] = make_value_int(-1);

        return vm_status_pending;
    }

    FileStream *fs = (FileStream *)fs_object_ptr;
    int done_rc = 0;

    for (; fs_object_ptr->as_bool(fs_object_ptr) && rc > 0; rc--, done_rc++) {
        dest_buf_ptr->set_v(dest_buf_ptr, make_value_none(), make_value_int(
            fgetc(fs->f)
        ));
    }

    s->sp++;
    s->stack[s->sp] = make_value_int(done_rc);

    return vm_status_pending;
}

VMStatus native_fwrite(VMState *s) {
    const int callee_bp = s->bp;
    const Value fs_arg = s->stack[callee_bp + 1];
    const Value src_arg = s->stack[callee_bp + 2];
    const Value rc_arg = s->stack[callee_bp + 2];

    if (fs_arg.tag != vtag_obj_id || src_arg.tag != vtag_obj_id || rc_arg.tag != vtag_int) {
        s->sp++;
        s->stack[s->sp] = make_value_int(-1);

        return vm_status_pending;
    }

    ObjPtr fs_object_ptr = heap_get(&s->heap, fs_arg.data.obj_id);
    ObjPtr src_buf_ptr = heap_get(&s->heap, src_arg.data.obj_id);
    int wc = rc_arg.data.i;

    if (!fs_object_ptr || fs_object_ptr->meta.tag != otag_fs
        || !src_buf_ptr || src_buf_ptr->meta.tag != otag_list
        || wc < 0) {
        s->sp++;
        s->stack[s->sp] = make_value_int(-1);

        return vm_status_pending;
    }

    FileStream *fs = (FileStream *)fs_object_ptr;
    List *src_buf = (List *)src_buf_ptr;
    int done_wc = 0;

    for (; fs_object_ptr->as_bool(fs_object_ptr) && wc > 0; wc--, done_wc++) {
        const Value *c_code = src_buf->data.data + wc;

        if (c_code->tag == vtag_int) {   
            fputc(c_code->data.i, fs->f);
        } else {
            fprintf(stderr, "\x1b[1;33mWARNING:\x1b[0m ~ tb_io_stdlib.c, native_fwrite():\nInvalid value at position %d, stopped on non-integer.\n", wc);
            break;
        }
    }

    s->sp++;
    s->stack[s->sp] = make_value_int(done_wc);

    return vm_status_pending;
}
