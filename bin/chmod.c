#include <inc/lib.h>

void
umain(int argc, char **argv) {
    binaryname = "chmod";

    if (argc != 3) {
        printf("chmod: wrong number of arguments\n");
        return;
    }

    if (strnlen(argv[2], MAXPATHLEN) != 3) {
        printf("chmod: wrong mode\n");
        return;
    }
    int mode = 0;
    mode |= (argv[2][0] - '0') << 6;
    mode |= (argv[2][1] - '0') << 3;
    mode |= (argv[2][2] - '0');
    printf("mode = %o\n", mode);
        
    int res = chmod(argv[1], mode);
    if (res < 0) {
        printf("chmod: error %i(%d)\n", res, res);
    }
}

