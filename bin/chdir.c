#include <inc/lib.h>

// FIXME: Not work
void
umain(int argc, char **argv) {
    binaryname = "chdir";

    if (argc != 2) {
        printf("chdir: wrong number of arguments\n");
        return;
    }
    printf("<%s>\n", argv[1]);

    int res = set_cwd(argv[1]);
    if (res < 0) {
        printf("chdir: error %i(%d)", res, res);
    }
}
