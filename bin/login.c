#include <inc/lib.h>
#include <inc/stdio.h>
#include <inc/crypto.h>


// TODO: add shadow file check
const char* kPasswdPath = "/passwd";
const char* kShadowPath = "/shadow";

#define MAX_LOGIN_AND_PASSWORD_LENGTH 256
#define MAX_LINE_BUFF_LENGTH          (MAX_LOGIN_AND_PASSWORD_LENGTH * 2 + 256)


static void help(void);
static void no_users_login(void);
static bool login(void);

/* Parsed line of etc/passwd
   username:UID:GUI:homedir:shell

   P.s. Must be implemented as array of char*
*/
struct PasswParsed {
    const char* username;
    const char* uid;
    const char* guid;
    const char* homedir;
    const char* shell;
};

/* Parsed line of etc/passwrd
   username:salt:hashed:last_change_date
*/
struct ShadowParsed {
    const char* username;
    const char* salt;
    const char* hashed;
    const char* last_change_date;
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

    if ((r = stat(kPasswdPath, &passwd_stat)) < 0) {
        printf("Can't open '%s'\n", kPasswdPath);

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
    char login[MAX_LOGIN_AND_PASSWORD_LENGTH];
    char password[MAX_LOGIN_AND_PASSWORD_LENGTH];

    strncpy(login, readline("Enter login: "), MAX_LOGIN_AND_PASSWORD_LENGTH);
    strncpy(password, readline_noecho("Enter password: "), MAX_LOGIN_AND_PASSWORD_LENGTH);

    if (1) {
        printf("\nlogin: %s\npassword: %s\n", login, password);
    }

    struct Fd* fpasswd = fopen(kPasswdPath, O_RDONLY);
    assert(fpasswd);

    struct Fd* fshadow = fopen(kShadowPath, O_RDONLY);
    assert(fshadow);

    char passwd_line_buf[MAX_LINE_BUFF_LENGTH];
    char shadow_line_buf[MAX_LINE_BUFF_LENGTH];


    while (fgets(passwd_line_buf, MAX_LINE_BUFF_LENGTH, fpasswd)) {
        fgets(shadow_line_buf, MAX_LINE_BUFF_LENGTH, fshadow);

        struct PasswParsed parsed_passwd_line;
        struct ShadowParsed parsed_shadow_line;

        parse_passw_line(&parsed_passwd_line, passwd_line_buf);
        parse_shadow_line(&parsed_shadow_line, shadow_line_buf);

        if (1) {
            printf("password: %s, hashed_password: %s\n", password, parsed_shadow_line.hashed);
        }

        if (strncmp(parsed_passwd_line.username, login, strlen(login))) {
            continue;
        }

        if (true == check_PBKDF2(parsed_shadow_line.hashed, parsed_shadow_line.salt, password)) {
            spawnl(parsed_passwd_line.shell, parsed_passwd_line.shell, NULL);

            printf("Hello '%s', welcome back!\n", login);
            return true;
        }
    }

    printf("Wrong login '%s' or password\n", login);
    return false;
}
