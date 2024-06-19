#include <inc/lib.h>

const char* directory = "/great_directory_52";
const char* file      = "/great_directory_52/very_great_directory_52";

void test_mkdir() {
    cprintf("\n==========================\nSTART TEST MKDIR\n==========================\n");
    int res = 0;

    res = mkdir(directory);
    cprintf("MKDIR TEST: During mkdir \"%s\" <%i>(%d)\n", directory, res, res);
 
    int fd = open(file, O_RDWR | O_CREAT);
    cprintf("MKDIR TEST: open \"%s\" <%i>(%d)\n", file, fd, fd);
    cprintf("fd after open \"%s\" %d\n", file, fd);
    cprintf("\n==========================\nEND TEST MKDIR\n==========================\n");
}

void
umain(int argc, char **argv) {
}
