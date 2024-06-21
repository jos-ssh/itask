#include "inc/convert.h"
#include "inc/env.h"
#include "inc/error.h"
#include "inc/fs.h"
#include "inc/mmu.h"
#include "kmod/file/cwd.h"
#include <inc/kmod/request.h>
#include <inc/kmod/file.h>
#include <inc/kmod/init.h>
#include <inc/kmod/users.h>
#include <inc/rpc.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/lib.h>


#ifndef debug_kfile
#define debug_kfile 1
#endif

#define KFILE_LOG(...) \
    if(debug_kfile) {                                                                     \
        cprintf("\e[32mKFILE_LOG\e[0m[\e[94m%s\e[0m:%d]: ", __func__, __LINE__); \
        cprintf(__VA_ARGS__);                                                \
    }
    
static union FiledRequest Request;
static union FiledResponse Response;

static uint8_t FdBuffer[PAGE_SIZE] __attribute__((aligned(PAGE_SIZE)));
static uint8_t sUsersdResponseBuffer[PAGE_SIZE] __attribute__((aligned(PAGE_SIZE)));

static union Fsipc FsBuffer;
static union InitdRequest InitdBuffer;
static union UsersdRequest sUsersdBuffer;

static envid_t
get_fs() {
    static envid_t fsenv;

    if (!fsenv) fsenv = ipc_find_env(ENV_TYPE_FS);

    return fsenv;
}

static int
fs_rpc_execute(unsigned type, union Fsipc* req, void* resp, int* perm) {
    ipc_send(get_fs(), type, req, PAGE_SIZE, PROT_RW);
    size_t maxsz = PAGE_SIZE;
    return ipc_recv_from(get_fs(), resp, &maxsz, perm);
};

static envid_t initd = 0;

static int filed_serve_identify(envid_t from, const void* request,
                                void* response, int* response_perm);

static int filed_serve_open(envid_t from, const void* request,
                            void* response, int* response_perm);
static int filed_serve_spawn(envid_t from, const void* request,
                             void* response, int* response_perm);
static int filed_serve_fork(envid_t from, const void* request,
                            void* response, int* response_perm);
static int filed_serve_remove(envid_t from, const void* request,
                              void* response, int* response_perm);
static int filed_serve_stat(envid_t from, const void* request,
                            void* response, int* response_perm);
static int filed_serve_chmod(envid_t from, const void* request,
                             void* response, int* response_perm);
static int filed_serve_chown(envid_t from, const void* request,
                             void* response, int* response_perm);
static int filed_serve_getcwd(envid_t from, const void* request,
                              void* response, int* response_perm);
static int filed_serve_setcwd(envid_t from, const void* request,
                              void* response, int* response_perm);

struct RpcServer Server = {
        .ReceiveBuffer = &Request,
        .SendBuffer = &Response,
        .Fallback = NULL,
        .HandlerCount = FILED_NREQUESTS,
        .Handlers = {
                [FILED_REQ_IDENTIFY] = filed_serve_identify,
                [FILED_REQ_OPEN] = filed_serve_open,
                [FILED_REQ_SPAWN] = filed_serve_spawn,
                [FILED_REQ_FORK] = filed_serve_fork,
                [FILED_REQ_REMOVE] = filed_serve_remove,
                [FILED_REQ_CHMOD] = NULL, // TODO: implement
                [FILED_REQ_CHOWN] = NULL, // TODO: implement
                [FILED_REQ_GETCWD] = filed_serve_getcwd,
                [FILED_REQ_SETCWD] = filed_serve_setcwd,

                [FILED_NREQUESTS] = NULL}};

void
umain(int argc, char** argv) {
    int res = sys_unmap_region(CURENVID, &Request, sizeof(Request));
    assert(res == 0);
    res = sys_unmap_region(CURENVID, FdBuffer, sizeof(FdBuffer));
    assert(res == 0);
    res = sys_unmap_region(CURENVID, sUsersdResponseBuffer, sizeof(sUsersdResponseBuffer));
    assert(res == 0);

    assert(argc >= 2);
    unsigned long cvt = 0;
    res = str_to_ulong(argv[1], BASE_HEX, &cvt);
    assert(res == 0);
    initd = cvt;

    cprintf("[%08x: filed] Starting up module...\n", thisenv->env_id);
    while (1) {
        rpc_listen(&Server, NULL);
    }
}

int
filed_serve_identify(envid_t from, const void* request,
                     void* response, int* response_perm) {
    union KmodIdentifyResponse* ident = response;
    memset(ident, 0, sizeof(*ident));
    ident->info.version = FILED_VERSION;
    strncpy(ident->info.name, FILED_MODNAME, MAXNAMELEN);
    *response_perm = PROT_R;
    return 0;
}

static int get_env_info(envid_t target, struct EnvInfo* info) {
    sUsersdBuffer.get_env_info.target = target;
    
    void* response = &sUsersdResponseBuffer;
    int res = rpc_execute(kmod_find_any_version(USERSD_MODNAME), USERSD_REQ_GET_ENV_INFO, &sUsersdBuffer, &response);
    if (res){
        KFILE_LOG("%i\n", res);
        return res;
    }

    if (!response) {
        KFILE_LOG("unspecified, %i", res);
        return -E_UNSPECIFIED;
    }

    assert(response == &sUsersdResponseBuffer);

    memcpy(info, response, sizeof(*info));

    KFILE_LOG("%s: [%x] uid: %d\n", __func__, target, info->euid);
    return 0;
}


static int get_file_info(int fileid, struct Fsret_stat* buffer) {
    static uint8_t sStatBuffer[PAGE_SIZE] __attribute__((aligned(PAGE_SIZE)));
    
    int temp = sys_unmap_region(CURENVID, sStatBuffer, PAGE_SIZE);
    assert(temp == 0);

    FsBuffer.stat.req_fileid = fileid; 
    KFILE_LOG("Stat of %d\n", FsBuffer.stat.req_fileid);

    int perm;
    int res = fs_rpc_execute(FSREQ_STAT, &FsBuffer, sStatBuffer, &perm);
    if (res) {
        KFILE_LOG("%i\n", res);
        return res;
    }
    assert(perm | PROT_R);

    memcpy(buffer, sStatBuffer, sizeof(*buffer));
    return 0;
}


static int check_perm (const struct EnvInfo* info, int fileid, int flags) {
    int res;
    
    struct Fsret_stat stat;
    res = get_file_info(fileid, &stat);
    if (res)
        return res;

    if (stat.ret_uid == info->euid) {
        // owner
        if (!(IRUSR | stat.ret_mode) && (!(flags | O_RDONLY) || (flags | O_RDWR)))
            res = -E_PERM_DENIED;

        if (!(IWUSR | stat.ret_mode) && ((flags | O_WRONLY) || (flags | O_RDWR)))
            res = -E_PERM_DENIED;

    } else if (stat.ret_gid == info->egid) {
        // group
        if (!(IRGRP | stat.ret_mode) && (!(flags | O_RDONLY) || (flags | O_RDWR)))
            return -E_PERM_DENIED;

        if (!(IWGRP | stat.ret_mode) && ((flags | O_WRONLY) || (flags | O_RDWR)))
            return -E_PERM_DENIED;

    } else {
        // others
        if (!(IROTH | stat.ret_mode) && (!(flags | O_RDONLY) || (flags | O_RDWR)))
            return -E_PERM_DENIED;

        if (!(IWOTH | stat.ret_mode) && ((flags | O_WRONLY) || (flags | O_RDWR)))
            return -E_PERM_DENIED;
    }

    return 0;
}

static int set_env_info(envid_t child, envid_t parent, const struct EnvInfo* info) {
    sUsersdBuffer.register_env.child_pid = child;
    sUsersdBuffer.register_env.parent_pid = parent;

    memcpy(&sUsersdBuffer.register_env.desired_child_info, info, sizeof(*info));

    int res = rpc_execute(kmod_find_any_version(USERSD_MODNAME), USERSD_REQ_REG_ENV, &sUsersdBuffer, NULL);
    if (res){
        KFILE_LOG("%i\n", res);
    }

    return res;
}

static int
filed_serve_open(envid_t from, const void* request,
                 void* response, int* response_perm) {
    int res = 0;

    struct EnvInfo env_info;
    res = get_env_info(from, &env_info);
    if (res)
        return res;

    const union FiledRequest* freq = request;
    if (freq->open.req_fd_vaddr & (PAGE_SIZE - 1)) return -E_INVAL;
    if (freq->open.req_fd_vaddr >= MAX_USER_ADDRESS) return -E_INVAL;

    const char* abs_path = NULL;
    res = filed_get_absolute_path(from, freq->open.req_path, &abs_path);
    if (res < 0) return res;

    // set FsBuffer request
    memcpy(FsBuffer.open.req_path, abs_path, MAXPATHLEN);

    FsBuffer.open.req_oflags = freq->open.req_flags;
    if (freq->open.req_flags & O_CREAT) {
        FsBuffer.open.req_omode = freq->open.req_omode;
        
        FsBuffer.open.req_uid = env_info.euid;
        FsBuffer.open.req_gid = env_info.egid;
    }

    int perm = 0;
    res = fs_rpc_execute(FSREQ_OPEN, &FsBuffer, FdBuffer, &perm);
    if (res)
        goto exit;

    res = check_perm(&env_info, ((struct Fd*)FdBuffer)->fd_file.id, freq->open.req_flags);
    if (res)
        goto exit;

    assert(perm & PROT_R);
    res = sys_map_region(CURENVID, FdBuffer, from, (void*)freq->open.req_fd_vaddr,
                            PAGE_SIZE, perm);
    assert(res == 0);

exit:
    int temp = sys_unmap_region(CURENVID, FdBuffer, PAGE_SIZE);
    assert(temp == 0);

    return res;
}

static int
filed_serve_spawn(envid_t from, const void* request,
                  void* response, int* response_perm) {
    int res = 0;
    const union FiledRequest* freq = request;
    const char* abs_path = NULL;
    res = filed_get_absolute_path(from, freq->spawn.req_path, &abs_path);
    if (res < 0) return res;

    // Check Permissions 
    struct EnvInfo env_info;
    res = get_env_info(from, &env_info);
    if (res)
        return res;

    // set FsBuffer request
    memcpy(FsBuffer.open.req_path, abs_path, MAXPATHLEN);
    FsBuffer.open.req_oflags = O_RDONLY;

    int perm = 0;
    res = fs_rpc_execute(FSREQ_OPEN, &FsBuffer, FdBuffer, &perm);
    if (res) {
        int temp = sys_unmap_region(CURENVID, FdBuffer, PAGE_SIZE);
        assert(temp == 0);
        return res;
    }

    res = check_perm(&env_info, ((struct Fd*)FdBuffer)->fd_file.id, freq->open.req_flags);
    if (res) {
        int temp = sys_unmap_region(CURENVID, FdBuffer, PAGE_SIZE);
        assert(temp == 0);
        return res;
    }

    struct Fsret_stat stat;
    res = get_file_info(((struct Fd*)FdBuffer)->fd_file.id, &stat);
    if (res) {
        int temp = sys_unmap_region(CURENVID, FdBuffer, PAGE_SIZE);
        assert(temp == 0);
        return res;
    }

    int temp = sys_unmap_region(CURENVID, FdBuffer, PAGE_SIZE);
    assert(temp == 0);
    // end of checking permissions

    InitdBuffer.spawn.parent = from;
    memcpy(InitdBuffer.spawn.file, abs_path, MAXPATHLEN);
    InitdBuffer.spawn.argc = freq->spawn.req_argc;
    memcpy(InitdBuffer.spawn.argv, freq->spawn.req_argv, sizeof(InitdBuffer.spawn.argv));
    memcpy(InitdBuffer.spawn.strtab, freq->spawn.req_strtab, sizeof(InitdBuffer.spawn.strtab));

    res = rpc_execute(initd, INITD_REQ_SPAWN, &InitdBuffer, NULL);
    if (res < 0) return res;

    envid_t child = res;
    const char* parent_cwd = filed_get_env_cwd(from);
    if (parent_cwd) {
        filed_set_env_cwd(child, parent_cwd);
    }

    // set GID and UID for child
    struct EnvInfo child_info = {NOT_AN_ID, NOT_AN_ID, NOT_AN_ID, NOT_AN_ID};  
    if (!(stat.ret_mode | ISUID)) {
        child_info.euid = stat.ret_uid;
    }

    if (!(stat.ret_mode | ISGID)) {
        child_info.egid = stat.ret_gid;
    }
    
    res = set_env_info(child, from, &child_info);
    assert(res == 0);

    res = sys_env_set_status(child, ENV_RUNNABLE);
    assert(res == 0);

    return child;
}

static int
filed_serve_fork(envid_t from, const void* request,
                 void* response, int* response_perm) {
    int res = 0;
    InitdBuffer.fork.parent = from;
    res = rpc_execute(initd, INITD_REQ_FORK, &InitdBuffer, NULL);
    if (res < 0) return res;

    envid_t child = res;
    const char* parent_cwd = filed_get_env_cwd(from);
    if (parent_cwd) {
        filed_set_env_cwd(child, parent_cwd);
    }

    // set GID and UID for child
    struct EnvInfo child_info = {NOT_AN_ID, NOT_AN_ID, NOT_AN_ID, NOT_AN_ID};  
    
    res = set_env_info(child, from, &child_info);
    assert(res == 0);

    res = sys_env_set_status(child, ENV_RUNNABLE);
    assert(res == 0);

    return child;
}

static int
filed_serve_remove(envid_t from, const void* request,
                   void* response, int* response_perm) {
    int res = 0;
    const union FiledRequest* freq = request;

    const char* abs_path = NULL;
    res = filed_get_absolute_path(from, freq->remove.req_path, &abs_path);
    if (res < 0) return res;

    // TODO: check permissions
    memcpy(FsBuffer.remove.req_path, abs_path, MAXPATHLEN);

    return fs_rpc_execute(FSREQ_REMOVE, &FsBuffer, NULL, NULL);
}

static int
filed_serve_getcwd(envid_t from, const void* request,
                   void* response, int* response_perm) {
    union FiledResponse* fresp = response;

    const char* cwd = filed_get_env_cwd(from);
    if (!cwd) return -E_NO_CWD;

    strncpy(fresp->cwd, cwd, MAXPATHLEN);
    *response_perm = PROT_R;
    return 0;
}

static int
filed_serve_setcwd(envid_t from, const void* request,
                   void* response, int* response_perm) {
    int res = 0;
    const union FiledRequest* freq = request;

    const char* abs_path = NULL;
    res = filed_get_absolute_path(from, freq->remove.req_path, &abs_path);
    if (res < 0) return res;

    // TODO: check permissions
    filed_set_env_cwd(from, abs_path);
    return 0;
}
