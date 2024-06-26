#include <inc/lib.h>

int flag[256];

static void
del_user(const char* username) {
    // TODO:
}


static void
usage(void) {
    printf("usage: userdel [options] LOGIN\n\n"
           "options:\n"
           "\n");
    exit();
}

void
umain(int argc, char** argv) {
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
    if (argc == 2) {
        del_user(argv[1]);
    } else {
        usage();
    }
}
