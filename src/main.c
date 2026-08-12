#include <stdio.h>
#include "tb_api.h"



int main(int argc, const char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Too few arguments. Try ./tbasic -i for information.\n");
        return 1;
    }

    const int exit_status = tbasic_run(argv, argc, NULL, NULL, 0);

    return exit_status;
}
