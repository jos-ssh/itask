#include <stdatomic.h>

#include <inc/rpc.h>
#include <inc/string.h>
#include <inc/lib.h>
#include <inc/convert.h>

#include "inc/env.h"
#include "inc/kmod/init.h"
#include "inc/kmod/request.h"
#include "inc/kmod/signal.h"
#include "signal.h"

struct SigdSharedData g_SharedData[NENV];

static union SigdRequest Request;
static union KmodIdentifyResponse Response;

static envid_t InitdEnvid;
static envid_t LoopEnvid;
static bool StartupDone = false;

static int sigd_serve_identify(envid_t from, const void* request,
                               void* response, int* response_perm);
static int sigd_start_loop(void);


struct RpcServer Server = {
        .ReceiveBuffer = &Request,
        .SendBuffer = &Response,
        .Fallback = NULL,
        .HandlerCount = SIGD_NREQUESTS,
        .Handlers = {
                [SIGD_REQ_IDENTIFY] = sigd_serve_identify,

                [SIGD_NREQUESTS] = NULL}};

void
umain(int argc, char** argv) {
    cprintf("[%08x: sigd] Starting up module...\n", thisenv->env_id);

    assert(argc > 1);
    unsigned long initd = 0;
    int cvt_res = str_to_ulong(argv[1], BASE_HEX, &initd);
    assert(cvt_res == 0);
    InitdEnvid = initd;


    for (;;) {
        bool already_done = StartupDone;
        rpc_listen(&Server, NULL);

        if (!already_done && StartupDone) {
            int res = sigd_start_loop();
            assert(res > 0);
            LoopEnvid = res;
        }
    }
}

int
sigd_serve_identify(envid_t from, const void* request,
                    void* response, int* response_perm) {
    union KmodIdentifyResponse* ident = response;
    memset(ident, 0, sizeof(*ident));
    ident->info.version = SIGD_VERSION;
    strncpy(ident->info.name, SIGD_MODNAME, KMOD_MAXNAMELEN);
    *response_perm = PROT_R;

    if (from == InitdEnvid) {
        StartupDone = true;
    }

    return 0;
}

static int
sigd_start_loop(void) {
    static union InitdRequest initd_req;
    void* res_data = NULL;

    initd_req.fork.parent = CURENVID;
    rpc_execute(InitdEnvid, INITD_REQ_FORK, &initd_req, &res_data);
    thisenv = &envs[ENVX(sys_getenvid())];

    int res = thisenv->env_ipc_value;
    if (res < 0) return res;

    if (res == 0) {
        sigd_signal_loop();
        panic("Unreachable code");
    }

    envid_t child = res;

    static_assert(sizeof(g_SharedData) % PAGE_SIZE == 0, "Unaligned shared data");
    res = sys_map_region(CURENVID, g_SharedData, child, g_SharedData,
                         sizeof(g_SharedData), PROT_RW | PROT_SHARE);
    if (res < 0) goto error;

    atomic_store(&g_SharedData[ENVX(child)].env, child);

    res = sys_env_set_status(child, ENV_RUNNABLE);
    if (res < 0) goto error;

    return child;
error:
    sys_env_destroy(child);
    return res;
}
