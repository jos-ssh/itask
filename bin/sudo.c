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
    if(argc != 3) {
        printf("requires 2 argument 'cmd' & 'argument'\n");
        return;
    }

    if(!login())
        return;

    printf("spawning '%s' with '%s'\n", argv[1], argv[2]);
    envid_t env = spawnl(argv[1], argv[1], argv[2]);

    wait(env);
    return;
}


static void
help(void) {
    printf("TODO: write help\n");
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
    strncpy(request.login.password, readline_noecho("Enter password: "), MAX_PASSWORD_LENGTH);
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
        printf("Wrong password for '%s'\n", request.login.username);
        return false;
    }

    memcpy(&info, &response->env_info.info, sizeof(info));
    sys_unmap_region(CURENVID, (void*)RECEIVE_ADDR, PAGE_SIZE);

    printf("Hello '%s', welcome back!\n", request.login.username);

    request.set_env_info.info.euid = ROOT_UID;
    request.set_env_info.info.ruid = ROOT_UID;
    request.set_env_info.info.egid = ROOT_GID;
    request.set_env_info.info.rgid = ROOT_GID;

    res = rpc_execute(sUsersService, USERSD_REQ_SET_ENV_INFO, &request, NULL);
    assert(res == 0);
    return true;
}
