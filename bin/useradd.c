#include <inc/lib.h>

int flag[256];

void
usage(void) {
    printf("usage: useradd [options] LOGIN\n\n"
           "options:\n"
           "  -d          home directory of the new account\n"
           "\n");
    exit();
}

void
umain(int argc, char **argv) {
    int i;
    struct Argstate args;

    argstart(&argc, argv, &args);
    while ((i = argnext(&args)) >= 0) {
        switch (i) {
        case 'd':
            flag[i]++;
            break;
        case 'h':
        default:
            usage();
        }
    }
}
