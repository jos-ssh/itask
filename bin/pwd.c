#include <inc/lib.h>

void
umain(int argc, char **argv) {
    binaryname = "pwd";

    if (argc != 1) {
        printf("pwd: wrong number of arguments\n");
        return;
    }

    char pwd[MAXPATHLEN] = {}; 
    int res = get_cwd(pwd, MAXPATHLEN);
    if (res < 0) {
        printf("pwd: error %i(%d)\n", res, res);
        return;
    }

    printf("%s\n", pwd);
}

