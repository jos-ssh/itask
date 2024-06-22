#include <inc/kmod/users.h>
#include <inc/kmod/file.h>

#include <inc/rpc.h>
#include <inc/lib.h>
#include <inc/stdio.h>

#include <inc/passw.h>

const size_t kMaxLoginAttempts = 3;
#define RECEIVE_ADDR 0x0FFFF000

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

    while (true) {
        size_t attempt = 0;
        while (attempt++ < kMaxLoginAttempts && !login()) {}
        if (attempt == kMaxLoginAttempts) {
            // TODO:
            attempt = 0;
        }
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

int
find_line_u(const char* username, const char* path_to_file,
            char* buff, const size_t size, const char** result, const size_t n_of_fields) {

    struct Fd* file = fopen(path_to_file, O_RDONLY);
    if (!file)
        return -E_INVAL;

    int res = -E_NO_ENT;

    while (fgets(buff, size, file)) {
        parse_line(result, n_of_fields, buff);

        // result[0] must be username
        if (!strncmp(result[0], username, result[1] - result[0] - 1)) {
            res = 0;
            break;
        }
    }

    fclose(file);
    return res;
}

inline int
find_passw_line_u(const char* username, char* buff, size_t size,
                  struct PasswParsed* result) {
    return find_line_u(username, PASSWD_PATH, buff, size, (const char**)result, sizeof(*result) / sizeof(char*));
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

    union UsersdResponse* response = (void*)RECEIVE_ADDR;
    struct EnvInfo info;

    int uid = rpc_execute(sUsersService, USERSD_REQ_LOGIN, &request, (void**)&response);
    memcpy(&info, &response->env_info.info, sizeof(info));
    sys_unmap_region(CURENVID, (void*)RECEIVE_ADDR, PAGE_SIZE);


    if (uid == 0) {
        printf("Hello '%s', welcome back!\n", request.login.username);

        struct PasswParsed passw;
        char passwd_line_buf[kMaxLineBufLength];
        int res = find_passw_line_u(request.login.username, passwd_line_buf, kMaxLineBufLength, &passw);
        assert(res == 0);

        static union FiledRequest file_request;
        strncpy(file_request.setcwd.req_path, passw.homedir, passw.shell - passw.homedir - 1);
        res = rpc_execute(kmod_find_any_version(FILED_MODNAME), FILED_REQ_SETCWD, &file_request, NULL);
        assert(res == 0);

        request.set_env_info.info.euid = info.euid;
        request.set_env_info.info.ruid = info.ruid;
        request.set_env_info.info.egid = info.egid;
        request.set_env_info.info.rgid = info.rgid;

        res = rpc_execute(sUsersService, USERSD_REQ_SET_ENV_INFO, &request, NULL);
        assert(res == 0);

        envid_t env;

        char* shell_string_end = strchr(passw.shell, '\n');
        if (shell_string_end == NULL) {
            env = spawnl(passw.shell, passw.shell, NULL);
        } else {
            char shell_buf[kMaxLineBufLength];
            strncpy(shell_buf, passw.shell, shell_string_end - passw.shell);
            env = spawnl(shell_buf, shell_buf, NULL);
        }

        wait(env);
        return true;
    }

    printf("Wrong login '%s' or password\n", request.login.username);
    return false;
}
