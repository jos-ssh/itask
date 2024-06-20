#include <inc/kmod/users.h>
#include <inc/rpc.h>
#include <inc/lib.h>
#include <inc/stdio.h>

#include <inc/passw.h>

const size_t kMaxLoginAttempts = 3;

static void help(void);
static void no_users_login(void);
static bool login(void);

// Workaround for preventing overlapping of initd output
static void
sleep(int secons) {
    secons *= 1000;
    while (secons--) {
        sys_yield();
    }
}

void
umain(int argc, char** argv) {
    int r;

    /* Being run directly from kernel, so no file descriptors open yet */
    // TODO: check that no file descriptors *indeed* opened
    close(0);

    if ((r = opencons()) < 0)
        panic("opencons: %i", r);
    if (r != 0)
        panic("first opencons used fd %d", r);
    if ((r = dup(0, 1)) < 0)
        panic("dup: %i", r);

    struct Stat passwd_stat = {};

    if ((r = stat(PASSWD_PATH, &passwd_stat)) < 0) {
        printf("Can't open '%s'\n", PASSWD_PATH);

        if (debug)
            printf("%i\n", r);
        return;
    }

    if (passwd_stat.st_size == 0)
        return no_users_login();

    // workaround for prety console print
    sleep(1);


    size_t attempt = 0;
    while (attempt++ < kMaxLoginAttempts && !login()) {}
    if (attempt == kMaxLoginAttempts) {
        // TODO:
    }
}


static void
help(void) {
    printf("TODO: write help\n");
}

static void
no_users_login(void) {
    spawnl("/sh", "/sh", (char*)0);
}


static bool
login(void) {
    static union UsersdRequest request = {};

    strncpy(request.login.username, readline("Enter login: "), MAX_USERNAME_LENGTH);
    strncpy(request.login.password, readline_noecho("Enter password: "), MAX_PASSWORD_LENGTH);
    printf("\n");

    static envid_t sUsersService;
    if (!sUsersService) {
        sUsersService = kmod_find_any_version(USERSD_MODNAME);
        assert(sUsersService > 0);
    }

    void* dummy = NULL;
    int res = rpc_execute(sUsersService, USERSD_REQ_LOGIN, &request, &dummy);

    if (res == 0) {
        printf("Hello '%s', welcome back!\n", request.login.username);
        return true;
    }

    printf("Wrong login '%s' or password\n", request.login.username);
    return false;
}
