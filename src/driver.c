#include <stdio.h>
#include <sys/time.h>

#include "driver.h"



static const LexItem special_lexicals_v[] = {
    (LexItem) {.literal = "NIL", .tag = tk_none},
    (LexItem) {.literal = "TRUE", .tag = tk_true},
    (LexItem) {.literal = "FALSE", .tag = tk_false},
    (LexItem) {.literal = "LET", .tag = tk_keyword_let},
    (LexItem) {.literal = "IF", .tag = tk_keyword_if},
    (LexItem) {.literal = "ELSE", .tag = tk_keyword_else},
    (LexItem) {.literal = "WHILE", .tag = tk_keyword_while},
    (LexItem) {.literal = "FOR", .tag = tk_keyword_for},
    (LexItem) {.literal = "BREAK", .tag = tk_keyword_break},
    (LexItem) {.literal = "CONTINUE", .tag = tk_keyword_continue},
    (LexItem) {.literal = "RET", .tag = tk_keyword_ret},
    (LexItem) {.literal = "FUN", .tag = tk_keyword_fun},
    (LexItem) {.literal = "END", .tag = tk_keyword_end},
    (LexItem) {.literal = "*", .tag = tk_os_times},
    (LexItem) {.literal = "/", .tag = tk_os_slash},
    (LexItem) {.literal = "+", .tag = tk_os_plus},
    (LexItem) {.literal = "-", .tag = tk_os_minus},
    (LexItem) {.literal = "==", .tag = tk_os_equals},
    (LexItem) {.literal = "!=", .tag = tk_os_bang_equals},
    (LexItem) {.literal = "<", .tag = tk_os_lesser},
    (LexItem) {.literal = ">", .tag = tk_os_greater},
    (LexItem) {.literal = "&&", .tag = tk_os_and},
    (LexItem) {.literal = "||", .tag = tk_os_or},
    (LexItem) {.literal = ":=", .tag = tk_os_bind_equals},
    (LexItem) {.literal = ":", .tag = tk_colon}
};

IMPL_SCALAR_VEC(NativeFn)

mystr read_file(const char *fname) {
    FILE *fs = fopen(fname, "r");

    if (!fs) {
        fprintf(stderr, "Read Error: The file at \x1b[1;29m%s\x1b[0m could not be read.", fname);
        return (mystr) {
            .data = NULL,
            .capacity = 0,
            .length = 0
        };
    }

    char chunk[64];
    mystr source;
    mystr_res(&source, 64);

    size_t total_rc = 0;
    size_t rc = 0;

    while (1) {
        rc = fread(chunk, sizeof(char), 32, fs);

        if (rc == 0) {
            break;
        }

        total_rc += rc;

        chunk[rc] = '\0';
        mystr_append_raw(&source, chunk, rc);
    }

    if (ferror(fs)) {
        fprintf(stderr, "File read failed.\n");
        return (mystr) {
            .data = NULL,
            .capacity = 0,
            .length = 0
        };
    } else if (total_rc == 0) {
        mystr_del(&source);
    }

    fclose(fs);
    return source;
}



void driver_dud(Driver *d, const DriverConfig *config) {
    d->compiler = make_compiler();
    ScalarVec_NativeFn_dud(&d->natives);
    d->config = *config;
    memset(&d->flags, 0, sizeof(d->flags));
}

void driver_del(Driver *d) {
    compiler_del(&d->compiler);
    ScalarVec_NativeFn_del(&d->natives);
}

void driver_bind_native(Driver *d, charspan name, NativeFn fn) {
    ScalarVec_NativeFn_push(&d->natives, fn);
    compiler_map_native(&d->compiler, &name);
}

Program driver_compile(Driver *d, const char *file_path) {
    mystr file_source = read_file(file_path);

    if (mystr_empty(&file_source)) {
        return (Program) {
            .chunks = {NULL, 0, 0},
            .strings = {NULL, 0, 0},
            .entry_id = -1
        };
    }

    Program program;
    program_dud(&program);

    charspan source_view;
    charspan_new(&source_view, file_source.data, file_source.length);

    Lexer tokenizer = make_lexer(&source_view, special_lexicals_v);

    if (!compiler_do_source(&d->compiler, &tokenizer, &source_view, &program)) {
        charspan_del(&source_view);
        program_del(&program);
        mystr_del(&file_source);

        return (Program) {
            .chunks = {NULL, 0, 0},
            .strings = {NULL, 0, 0},
            .entry_id = -1
        };
    }

    charspan_del(&source_view);
    mystr_del(&file_source);

    return program;
}

int driver_run(Driver *d, const char *file_path) {
    if (driver_get_flag(d, dflag_info)) {
        printf(
            "\x1b[1;34m%s\x1b[0m\n\x1b[1;29mv%d.%d.%d\x1b[0m  \x1b[1;30m---\x1b[0m  \x1b[1;29mDrkWithT (GitHub)\x1b[0m\n",
            d->config.title,
            d->config.version_major, d->config.version_minor, d->config.version_patch
        );
        printf("usage: ./tbasic [-i | [-d | -r] <file name>]\n\t-i: show usage and version\n");
        return 0;
    }

    Program code = driver_compile(d, file_path);

    if (code.entry_id == -1) {
        return 1;
    }

    // TODO: add usage of other driver flags: dump / run bytecode files?
    if (driver_get_flag(d, dflag_dis_bc)) {
        dump_program(&code);
        program_del(&code);
        return 0;
    }

    if (!driver_get_flag(d, dflag_run_bc)) {
        dump_program(&code);
        program_del(&code);
        return 0;
    }

    VMState vm = make_vm(&code, d->natives.data, d->config.stack_capacity, d->config.recursion_max, d->config.heap_capacity);

    struct timeval begin, end;

    gettimeofday(&begin, NULL);
    const VMStatus status = vm_run(&vm);
    gettimeofday(&end, NULL);

    puts("Result:");
    const Value ans = vm_result(&vm);
    print_value(&ans, &vm);

    const long end_usec = end.tv_sec * 1000000 + end.tv_usec;
    const long begin_usec = begin.tv_sec * 1000000 + begin.tv_usec;
    printf("\nDONE in %zu ms\n", (end_usec - begin_usec) / 1000);

    dispose_vm(&vm);
    program_del(&code);

    return (status == vm_status_ok) ? 0 : 1;
}
