#include <inc/kmod/users.h>
#include <inc/kmod/init.h>

#include <inc/rpc.h>
#include <inc/crypto.h>
#include <inc/random.h>
#include <inc/passw.h>
#include <inc/lib.h>

const size_t kMaxDelay = 1000;

#ifndef debug_kusers
#define debug_kusers 0
#endif

#define KUSERS_LOG(...) \
    if(debug_kusers) {                                                                     \
        cprintf("\e[36mKUSERS_LOG\e[0m[\e[94m%s\e[0m:%d]: ", __func__, __LINE__); \
        cprintf(__VA_ARGS__);                                                \
    }

static int usersd_serve_identify(envid_t from, const void* request,
                                 void* response, int* response_perm);

static int usersd_serve_login(envid_t from, const void* request,
                              void* response, int* response_perm);

static int usersd_serve_register_env(envid_t from, const void* request,
                                     void* response, int* response_perm);

static int usersd_serve_get_env_info(envid_t from, const void* request,
                                     void* response, int* response_perm);

static int usersd_serve_set_env_info(envid_t from, const void* request,
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
                [USERSD_REQ_SET_ENV_INFO] = usersd_serve_set_env_info,
        }};

static void
set_env_info(struct EnvInfo* child, const struct EnvInfo* parent, const struct EnvInfo* desired) {
    child->ruid = (desired && desired->ruid != NOT_AN_ID) ? desired->ruid : parent->ruid;
    child->euid = (desired && desired->euid != NOT_AN_ID) ? desired->euid : parent->euid;

    child->rgid = (desired && desired->rgid != NOT_AN_ID) ? desired->rgid : parent->rgid;
    child->egid = (desired && desired->egid != NOT_AN_ID) ? desired->egid : parent->egid;
}

struct FullEnvInfo {
    envid_t pid;
    struct EnvInfo info;
};

struct FullEnvInfo gEnvsInfo[NENV];

enum ModuleStatus {
    kJustStarted,
    kAuthorized, // when replied to usersd_serve_identify
    kSpawnedLogin,
};

static void start_login(void) {
    cprintf("[%08x: usersd] Starting up module...\n", thisenv->env_id);
    
    static union InitdRequest req;
    req.spawn.parent = thisenv->env_id;
    strcpy(req.spawn.file, "/login");
    req.spawn.argc = 0;
    envid_t login = rpc_execute(kmod_find_any_version(INITD_MODNAME), INITD_REQ_SPAWN, &req, NULL);
    KUSERS_LOG("spawned login %x\n", login);

    gEnvsInfo[ENVX(thisenv->env_id)].pid = thisenv->env_id;
    gEnvsInfo[ENVX(thisenv->env_id)].info.ruid = ROOT_UID;
    gEnvsInfo[ENVX(thisenv->env_id)].info.euid = ROOT_UID;
    gEnvsInfo[ENVX(thisenv->env_id)].info.rgid = ROOT_GID;
    gEnvsInfo[ENVX(thisenv->env_id)].info.egid = ROOT_GID;

    struct UsersdRegisterEnv reg_login = {thisenv->env_id, login, {}}; 
    int res = usersd_serve_register_env(thisenv->env_id, &reg_login, NULL, NULL); 
    assert(res == 0);

    res = sys_env_set_status(login, ENV_RUNNABLE);
    assert(res == 0);
    KUSERS_LOG("started login %x\n", login);
}

static enum ModuleStatus gModuleStatus = kJustStarted;
void
umain(int argc, char** argv) {
    while (1) {
        if (gModuleStatus == kAuthorized) {
            start_login();
            gModuleStatus = kSpawnedLogin;
        }

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

    gModuleStatus = kAuthorized;
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
    KUSERS_LOG("%s %s\n", req->username, req->password);
    // if (sCurrentUser)
        // return -E_ALREADY_LOGGED_IN;

    char passwd_line_buf[kMaxLineBufLength];
    char shadow_line_buf[kMaxLineBufLength];

    struct PasswParsed passw;
    struct ShadowParsed shadow;

    int res1 = find_passw_line(req->username, passwd_line_buf, kMaxLineBufLength, &passw);
    int res2 = find_shadow_line(req->username, shadow_line_buf, kMaxLineBufLength, &shadow);

    if (res1 == 0 && res2 == 0) {
        // Jos Security
        sleep(rand() & kMaxDelay);

        if (sys_crypto(shadow.hashed, shadow.salt, req->password)) {
            sCurrentUser = strtol(passw.uid, NULL, 10);
            return 0;
        }
    }

    return -E_ACCESS_DENIED;
}



static bool
is_env_info_empty(const struct EnvInfo* info) {
    return info->ruid == NOT_AN_ID &&
           info->euid == NOT_AN_ID &&
           info->rgid == NOT_AN_ID &&
           info->egid == NOT_AN_ID;
}

static int
usersd_serve_register_env(envid_t from, const void* request,
                          void* response, int* response_perm) {
    if (!request)
        return -E_INVAL;

    const struct UsersdRegisterEnv* req = request;
    KUSERS_LOG("child_pid %x, parent_pid %x, desired euid %d\n", req->child_pid, req->parent_pid, req->desired_child_info.euid);

    if (req->parent_pid < 0 || req->child_pid < 0)
        return -E_INVAL;

    struct FullEnvInfo* parent_info = gEnvsInfo + ENVX(req->parent_pid);
    struct FullEnvInfo* child_info = gEnvsInfo + ENVX(req->child_pid);

    if (parent_info->pid != req->parent_pid) // parent isn't valid
        return -E_BAD_ENV;

    if (child_info->pid == req->child_pid)
        return -E_ENV_ALREADY_REGISTERED;

    child_info->pid = req->child_pid;

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
    KUSERS_LOG("from %x req about %x\n", from, req->target);
    struct FullEnvInfo* info = gEnvsInfo + ENVX(req->target);
    KUSERS_LOG("target %x pid %x: egid %d\n", req->target, info->pid, info->info.egid);
    if (req->target != info->pid)
        return -E_BAD_ENV;

    struct UsersdEnvInfo* res = response;

    memcpy(&res->info, &info->info, sizeof(*res));
    *response_perm = PROT_R;

    return 0;
}

static int
usersd_serve_set_env_info(envid_t from, const void* request,
                          void* response, int* response_perm) {
    const struct UsersdSetEnvInfo* req = request;
    KUSERS_LOG("from %x\n", from);

    struct FullEnvInfo* info = gEnvsInfo + ENVX(from);
    KUSERS_LOG("target %x pid %x: egid %d\n", from, info->pid, info->info.egid);
    if (from != info->pid)
        return -E_BAD_ENV;

    if (info->info.euid != ROOT_UID)
        return -E_NOT_ENOUGH_PRIVILEGES;

    set_env_info(&info->info, &info->info, &req->info);

    return 0;
}