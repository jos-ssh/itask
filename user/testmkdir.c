#include <inc/lib.h>

void
umain(int argc, char **argv) {
    int res = 0;
    res = open("/new_folder/hello", O_RDWR);
    cprintf("Res during open \"/new_folder/hello\" %i\n", res);

    res = mkdir("/new_folder");
    if (res < 0)
        panic("mkdir \"/new_folder\" %i", res);
    cprintf("mkdir \"/new_folder\"\n");

    int fd = open("/new_folder/hello", O_RDWR);
    if (fd < 0)
        open("mkdir \"/new_folder/hello\" %i", fd);
    cprintf("fd after open \"/new_folder/hello\" %i\n", res);
}
