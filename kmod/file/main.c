#include "inc/convert.h"
#include "inc/env.h"
#include "inc/error.h"
#include "inc/fs.h"
#include "inc/kmod/init.h"
#include "inc/mmu.h"
#include <inc/kmod/request.h>
#include <inc/kmod/file.h>
#include <inc/rpc.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/lib.h>

static union FiledRequest Request;
static union FiledResponse Response;

static uint8_t FdBuffer[PAGE_SIZE] __attribute__((aligned(PAGE_SIZE)));

static union Fsipc FsBuffer;
static union InitdRequest InitdBuffer;

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
                [FILED_REQ_REMOVE] = filed_serve_remove,

                [FILED_NREQUESTS] = NULL}};

void
umain(int argc, char** argv) {
    int res = sys_unmap_region(CURENVID, &Request, sizeof(Request));
    assert(res == 0);
    res = sys_unmap_region(CURENVID, FdBuffer, sizeof(FdBuffer));
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


static int
filed_serve_open(envid_t from, const void* request,
                 void* response, int* response_perm) {
    int res = 0;
    const union FiledRequest* freq = request;
    if (freq->open.req_fd_vaddr & (PAGE_SIZE - 1)) return -E_INVAL;
    if (freq->open.req_fd_vaddr >= MAX_USER_ADDRESS) return -E_INVAL;

    memcpy(FsBuffer.open.req_path, freq->open.req_path, MAXPATHLEN);
    FsBuffer.open.req_omode = freq->open.req_omode;

    int perm = 0;
    res = fs_rpc_execute(FSREQ_OPEN, &FsBuffer, FdBuffer, &perm);

    if (res == 0) {
        // TODO: check permissions with stat

        assert(perm & PROT_R);
        res = sys_map_region(CURENVID, FdBuffer, from, (void*)freq->open.req_fd_vaddr,
                             PAGE_SIZE, perm);
        assert(res == 0);
    }

    int status = res;
    res = sys_unmap_region(CURENVID, FdBuffer, PAGE_SIZE);
    assert(res == 0);

    return status;
}

static int
filed_serve_spawn(envid_t from, const void* request,
                  void* response, int* response_perm) {
    int res = 0;
    const union FiledRequest* freq = request;
    // TODO: check permissions

    InitdBuffer.spawn.parent = from;
    memcpy(InitdBuffer.spawn.file, freq->spawn.req_path, MAXPATHLEN);
    InitdBuffer.spawn.argc = freq->spawn.req_argc;
    memcpy(InitdBuffer.spawn.argv, freq->spawn.req_argv, sizeof(InitdBuffer.spawn.argv));
    memcpy(InitdBuffer.spawn.strtab, freq->spawn.req_strtab, sizeof(InitdBuffer.spawn.strtab));

    void* recv_data = NULL;
    res = rpc_execute(initd, INITD_REQ_SPAWN, &InitdBuffer, &recv_data);
    if (res < 0) return res;

    envid_t child = res;
    // TODO: set GID and UID for child
    res = sys_env_set_status(child, ENV_RUNNABLE);
    assert(res == 0);

    return child;
}

static int
filed_serve_remove(envid_t from, const void* request,
                   void* response, int* response_perm) {
    const union FiledRequest* freq = request;
    // TODO: check permissions

    memcpy(FsBuffer.remove.req_path, freq->remove.req_path, MAXPATHLEN);

    return fs_rpc_execute(FSREQ_REMOVE, &FsBuffer, NULL, NULL);
}
