#include <inc/kmod/users.h>
#include <inc/rpc.h>
#include <inc/crypto.h>
#include <inc/random.h>
#include <inc/passw.h>
#include <inc/lib.h>

const size_t kMaxDelay = 1000;
const size_t kMaxLineBufLength = 512;

static int usersd_serve_identify(envid_t from, const void* request,
                                 void* response, int* response_perm);

static int usersd_serve_login(envid_t from, const void* request,
                              void* response, int* response_perm);

static int usersd_serve_register_env(envid_t from, const void* request,
                                     void* response, int* response_perm);

static int usersd_serve_get_env_info(envid_t from, const void* request,
                                     void* response, int* response_perm);

#define RECEIVE_ADDR 0x0FFFF000
static union UsersdResponse ResponseBuffer;

struct RpcServer Server = {
        .ReceiveBuffer = (void*)RECEIVE_ADDR,
        .SendBuffer = &ResponseBuffer,
        .Fallback = NULL,
        .HandlerCount = USERSD_NREQUESTS,
        .Handlers = {
                [USERSD_REQ_IDENTIFY] = usersd_serve_identify,
                [USERSD_REQ_LOGIN] = usersd_serve_login,
                [USERSD_REQ_REG_ENV] = usersd_serve_register_env,
                [USERSD_REQ_GET_ENV_INFO] = usersd_serve_get_env_info,
        }};

void
umain(int argc, char** argv) {
    printf("[%08x: usersd] Starting up module...\n", thisenv->env_id);
    while (1) {
        rpc_listen(&Server, NULL);
    }
}

static int
usersd_serve_identify(envid_t from, const void* request,
                      void* response, int* response_perm) {
    union KmodIdentifyResponse* ident = response;
    memset(ident, 0, sizeof(*ident));
    ident->info.version = USERSD_VERSION;
    strncpy(ident->info.name, USERSD_MODNAME, MAXNAMELEN);
    *response_perm = PROT_R;
    return 0;
}

static void
sleep(int nanosecons) {
    while (nanosecons--) {
        // No-op
    }
}

static int
usersd_serve_login(envid_t from, const void* request,
                   void* response, int* response_perm) {
    srand(vsys_gettime());
    static uid_t sCurrentUser;

    const struct UsersdLogin* req = request;

    if (sCurrentUser)
        return -E_ALREADY_LOGGED_IN;

    char passwd_line_buf[kMaxLineBufLength];
    char shadow_line_buf[kMaxLineBufLength];

    struct PasswParsed passw;
    struct ShadowParsed shadow;

    int res1 = find_passw_line(req->username, passwd_line_buf, kMaxLineBufLength, &passw);
    int res2 = find_shadow_line(req->password, shadow_line_buf, kMaxLineBufLength, &shadow);

    if (res1 == 0 && res2 == 0) {
        // Jos Security
        sleep(rand() & kMaxDelay);

        if (check_PBKDF2(shadow.hashed, shadow.salt, req->password)) {
            spawnl(passw.shell, passw.shell, NULL);

            return 0;
        }
    }

    return -E_ACCESS_DENIED;
}

struct FullEnvInfo {
    envid_t pid;
    struct EnvInfo info;
};

struct FullEnvInfo gEnvsInfo[NENV];

static void
set_env_info(struct EnvInfo* child, const struct EnvInfo* parent, const struct EnvInfo* desired) {
    child->ruid = (desired && desired->ruid) ? desired->ruid : parent->ruid;
    child->euid = (desired && desired->euid) ? desired->euid : parent->euid;

    child->rgid = (desired && desired->rgid) ? desired->rgid : parent->rgid;
    child->egid = (desired && desired->egid) ? desired->egid : parent->egid;
}

static bool
is_env_info_empty(const struct EnvInfo* info) {
    return info->ruid == 0 &&
           info->euid == 0 &&
           info->rgid == 0 &&
           info->egid == 0;
}

static int
usersd_serve_register_env(envid_t from, const void* request,
                          void* response, int* response_perm) {
    if (!request)
        return -E_INVAL;

    const struct UsersdRegisterEnv* req = request;

    if (req->parent_pid < 0 || req->child_pid < 0)
        return -E_INVAL;

    struct FullEnvInfo* parent_info = gEnvsInfo + ENVX(req->parent_pid);
    struct FullEnvInfo* child_info = gEnvsInfo + ENVX(req->parent_pid);

    if (parent_info->pid != req->parent_pid) // parent isn't valid
        return -E_BAD_ENV;

    if (child_info->pid == req->child_pid)
        return -E_ENV_ALREADY_REGISTERED;

    if (is_env_info_empty(&req->desired_child_info)) {
        set_env_info(&child_info->info, &parent_info->info, NULL);
    } else {
        if (parent_info->info.euid != ROOT_UID)
            return -E_NOT_ENOUGH_PRIVILEGES;

        set_env_info(&child_info->info, &parent_info->info, &req->desired_child_info);
    }

    child_info->pid = req->child_pid;
    return 0;
}

static int
usersd_serve_get_env_info(envid_t from, const void* request,
                          void* response, int* response_perm) {
    const struct UsersdGetEnvInfo* req = request;
    struct FullEnvInfo* info = gEnvsInfo + ENVX(req->target);

    if (req->target != info->pid)
        return -E_BAD_ENV;

    struct UsersdEnvInfo* res = response;

    memcpy(&res->info, &info->info, sizeof(*res));
    *response_perm = PROT_R;

    return 0;
}