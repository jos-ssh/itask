#include <inc/kmod/users.h>
#include <inc/kmod/file.h>

#include <inc/rpc.h>
#include <inc/lib.h>
#include <inc/stdio.h>

#include <inc/passw.h>

#define RECEIVE_ADDR 0x0FFFF000

static void help(void);
static bool login(void);

void
umain(int argc, char** argv) {
    if (argc < 2) {
        help();
        return;
    }

    if (!login())
        return;

    envid_t env = spawn(argv[1], (const char**)argv + 1);
    if (env < 0) {
        printf("sudo: %i\n", env);
    }
    wait(env);
    return;
}


static void
help(void) {
    printf("sudo - execute a command as another user\n"
           "\n"
           "usage: sudo [command [arg ...]]\n");
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

    strncpy(request.login.username, "root", MAX_USERNAME_LENGTH);
    strncpy(request.login.password, readline_noecho("[sudo] password for root //TODO: "), MAX_PASSWORD_LENGTH);
    printf("\n");

    static envid_t sUsersService;
    if (!sUsersService) {
        sUsersService = kmod_find_any_version(USERSD_MODNAME);
        assert(sUsersService > 0);
    }

    union UsersdResponse* response = (void*)RECEIVE_ADDR;
    struct EnvInfo info;

    int res = rpc_execute(sUsersService, USERSD_REQ_LOGIN, &request, (void**)&response);
    if (res < 0) {
        printf("sudo: wrong password for '%s'\n", request.login.username);
        return false;
    }

    memcpy(&info, &response->env_info.info, sizeof(info));
    sys_unmap_region(CURENVID, (void*)RECEIVE_ADDR, PAGE_SIZE);

    request.set_env_info.info.euid = ROOT_UID;
    request.set_env_info.info.ruid = info.ruid;
    request.set_env_info.info.egid = ROOT_GID;
    request.set_env_info.info.rgid = info.rgid;

    res = rpc_execute(sUsersService, USERSD_REQ_SET_ENV_INFO, &request, NULL);
    assert(res == 0);
    return true;
}
