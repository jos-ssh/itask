#include <inc/lib.h>

void
umain(int argc, char **argv) {
    binaryname = "mkdir";

    if (argc != 2) {
        printf("mkdir: wrong number of arguments");
        return;
    }

    const int mode = 0777;
    int res = mkdir(argv[1], mode);

    if (res < 0) {
        printf("mkdir: error %i(%d)\n", res, res);
        return;
    }
}