#include <inc/lib.h>
#include <inc/stdio.h>
#include <inc/crypto.h>


// TODO: add shadow file check
const char* PASSWD_PATH = "/passwd";
#define MAX_LOGIN_AND_PASSWORD_LENGTH 256
#define MAX_PASSWD_LINE_BUFF_LENGTH   (MAX_LOGIN_AND_PASSWORD_LENGTH * 2 + 256)

static void help(void);
static void no_users_login(void);
static bool login(void);

/* Parsed line of etc/passed line
   username:encrypted_pass:UID:GUI:comment:homedir:shell

   P.s. Must be implemented as array of char*
*/

struct PasswParsed {
    const char* username;
    const char* pass;
    const char* uid;
    const char* guid;
    const char* comment;
    const char* homedir;
    const char* shell;
};

// TODO:: change \shadow format

struct ShadowParsed {
    const char* username;
    const char* pass;
    const char* date;
};

void
parse_passw_line(struct PasswParsed* p, const char* line) {
    assert(p);
    assert(line);

    const char** p_as_array = (const char**)p;

    for (int i = 0; i < sizeof(*p) / sizeof(*p_as_array); ++i) {
        p_as_array[i] = *line == ':' ? NULL : line;

        // printf("%s: %s\n", __func__, line);
        line = strchr(line, ':');
        if (line) ++line;
    }
}

void
parse_shadow_line(struct ShadowParsed* s, const char* line) {
    assert(s);
    assert(line);

    const char** p_as_array = (const char**)s;

    for (int i = 0; i < sizeof(*s) / sizeof(*p_as_array); ++i) {
        p_as_array[i] = *line == ':' ? NULL : line;

        // printf("%s: %s\n", __func__, line);
        line = strchr(line, ':');
        if (line) ++line;
    }
}

bool
authorize(const struct PasswParsed* p, const char* login, const char* passwd) {
    assert(p);
    assert(login);
    assert(passwd);

    if (strncmp(p->username, login, strlen(login)))
        return false;

    // TODO: add salt
    if (!check_PBKDF2(p->pass, "default", passwd)) {
        if (debug) {
            printf("Wrong password\n");
        }

        return false;
    }

    return true;
}

void
umain(int argc, char** argv) {
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

        if (debug)
            printf("%i\n", r);
        return;
    }

    if (passwd_stat.st_size == 0)
        return no_users_login();

    while (!login())
        ;

    return;
}


static void
help(void) {
    printf("TODO: write help\n");
}

static void
no_users_login(void) {
    spawnl("/sh", "sh", (char*)0);
}

static bool
login(void) {
    static char login_buf[MAX_LOGIN_AND_PASSWORD_LENGTH];
    static char passw_buf[MAX_LOGIN_AND_PASSWORD_LENGTH];

    strncpy(login_buf, readline("Enter login: "), MAX_LOGIN_AND_PASSWORD_LENGTH);
    strncpy(passw_buf, readline_noecho("Enter password: "), MAX_LOGIN_AND_PASSWORD_LENGTH);

    if (debug) {
        printf("login: %s\npassword: %s\n", login_buf, passw_buf);
    }

    struct Fd* fpassw = fopen(PASSWD_PATH, O_RDONLY);
    assert(fpassw);

    char line_buf[MAX_PASSWD_LINE_BUFF_LENGTH];

    while (fgets(line_buf, MAX_PASSWD_LINE_BUFF_LENGTH, fpassw)) {
        struct PasswParsed pass;
        parse_passw_line(&pass, line_buf);

        if (authorize(&pass, login_buf, passw_buf)) {
            spawnl(pass.shell, pass.shell, (char*)0);

            printf("Hello '%s', welcome back!\n", login_buf);
            return true;
        }
    }

    printf("Wrong login '%s' or password\n", login_buf);
    return false;
}
