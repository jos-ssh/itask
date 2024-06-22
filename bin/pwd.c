#include <inc/lib.h>

void
umain(int argc, char **argv) {
    binaryname = "pwd";

    if (argc != 1) {
        printf("pwd: wrong number of arguments");
        return;
    }

    char pwd[MAXPATHLEN] = {}; 
    // TODO implement
    printf("%s\n", pwd);
}

