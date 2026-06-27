#include <stdio.h>
#include "tb_api.h"

static const uint8_t add_two_bc[] = {
    84, 66, 67, 70, // ? magic prefix
    0x00,0x00, 0x00, 0x00, // ? main chunk at 0 (serialized u32) is bytecode of `addTwo(a, b)`
    0x00,0x00, 0x00, 0x00, // ? no constant strings
    // ! NOTE: `tbasic_push_int` is used to push operands for the bytecode later...
    // ? CHUNK 0: BC_LENGTH = (u64) 2, little endian of lowest to highest bytes, left to right...
    0x02, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00,
    19, // ? `ADD ...`
    0, 0 , 0,
    30, // ? `RET ...`
    0, 0, 0,
    // ? CHUNK 0: CONST_LENGTH = (u64) 0
    0x00,0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00,
    // ? DUD CHUNK: BC_LENGTH = (u64) 0, marks the end of data!
    0x00,0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00,
};

int main() {
    Program demo_pg = tbasic_deserialize_prgm(add_two_bc, sizeof(add_two_bc));

    Driver tbasic_instance = tbasic_make_driver(NULL, NULL, 0, &demo_pg);

    // ? SUM 11 and 31: This must be 42.
    const Value test_args_1[] = {
        (Value) {
            .data = {
                .i = 11
            },
            .tag = vtag_int
        },
        (Value) {
            .data = {
                .i = 31
            },
            .tag = vtag_int
        }
    };
    const Value test_sum_1 = tbasic_invoke(&tbasic_instance, test_args_1, 2);

    if (test_sum_1.tag == vtag_int && test_sum_1.data.i == 42) {
        printf("11 + 31 = %d\n", test_sum_1.data.i);
    } else {
        fprintf(stderr, "\x1b[1;31mDemo: unexpected sum value found of Value {.tag = %d, .data.i = %d}\x1b[0m\n", test_sum_1.tag, test_sum_1.data.i);
        driver_del(&tbasic_instance);
        program_del(&demo_pg);
        return 1;
    }

    // ? SUM -10 and 10: This must be 0.
    const Value test_args_2[] = {
        (Value) {
            .data = {
                .i = -10
            },
            .tag = vtag_int
        },
        (Value) {
            .data = {
                .i = 10
            },
            .tag = vtag_int
        }
    };
    const Value test_sum_2 = tbasic_invoke(&tbasic_instance, test_args_2, 2);

    if (test_sum_2.tag == vtag_int && test_sum_2.data.i == 0) {
        printf("10 - 10 = %d\n", test_sum_2.data.i);
    } else {
        fprintf(stderr, "\x1b[1;31mDemo: unexpected sum value found of Value {.tag = %d, .data.i = %d}\x1b[0m\n", test_sum_1.tag, test_sum_1.data.i);
        driver_del(&tbasic_instance);
        program_del(&demo_pg);
        return 1;
    }

    driver_del(&tbasic_instance);
    program_del(&demo_pg);
}