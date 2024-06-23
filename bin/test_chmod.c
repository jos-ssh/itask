#include <inc/lib.h>

void
umain(int argc, char **argv) {
    binaryname = "test_chmod";

    int res = open("mkdir", O_WRONLY);
    if (res < 0)
        printf("open with write failed\n");
    res = open("mkdir", O_RDONLY);
    if (res < 0)
        printf("open with read failed\n");
}
