#include <inc/lib.h>
#include <inc/stdio.h>

const char* PASSWD_PATH = "/passwd";
#define MAX_LOGIN_AND_PASSWORD_LENGTH 256

static void help(void);
static void no_users_login(void);
static void login(void);

void umain(int argc, char **argv) {
    int r;
    
    /* Being run directly from kernel, so no file descriptors open yet */
    // TODO check that no file descriptors *indeed* opened
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

        if(debug)
            printf("%i\n", r);

        panic("Access denied\n");
    }

    if (passwd_stat.st_size == 0)
        return no_users_login();
    
    return login();
}


static void help(void) {
    printf("TODO: write help\n");
}

static void no_users_login(void) {
    spawnl("/sh", "sh", (char*)0);
}

static void login(void) {
    static char login_buf[MAX_LOGIN_AND_PASSWORD_LENGTH];
    static char passw_buf[MAX_LOGIN_AND_PASSWORD_LENGTH];

    char* login = readline("Enter login: ");
    assert(login);

    strncpy(login_buf, login, MAX_LOGIN_AND_PASSWORD_LENGTH);
    
    char* passw = readline_noecho("Enter password: ");
    assert(passw);

    strncpy(passw_buf, passw, MAX_LOGIN_AND_PASSWORD_LENGTH);
    
    struct Fd* fpassw = fopen(PASSWD_PATH, O_RDONLY);
    assert(fpassw);

    char line_buf[256];
    while(true) {
        int res = 
}