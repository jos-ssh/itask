#include <inc/lib.h>

#include <inc/kmod/users.h>
#include <inc/rpc.h>

int flag[256];

void
usage(void) {
    printf("usage: useradd [options] LOGIN\n\n"
           "options:\n"
           "  -d          home directory of the new account\n"
           "\n");
    exit();
}


static void
add_user(const char *username) {
    union UsersdRequest request;

    strcpy(request.useradd.username, username);
    strcpy(request.useradd.homedir, "/");

    strcpy(request.useradd.passwd, readline_noecho("Enter password for new user: "));

    rpc_execute(kmod_find_any_version(USERSD_MODNAME), USERSD_REQ_USERADD, &request, NULL);
    printf("\n");
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

    add_user(argv[1]);
}
