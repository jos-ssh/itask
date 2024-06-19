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

struct SigdSharedData g_SharedData[NENV] __attribute__((aligned(PAGE_SIZE)));

struct SigdEnvData {
    envid_t env;
    uint64_t sigmask;

    uintptr_t handler_vaddr;
};
static struct SigdEnvData EnvData[NENV];

static envid_t InitdEnvid;
static envid_t LoopEnvid;
static bool StartupDone = false;

static int sigd_start_loop(void);
static void
sigd_reset_env_data(envid_t env) {
    size_t idx = ENVX(env);
    /* Clear shared data */
    atomic_store(&g_SharedData[idx].timer_countdown, 0);
    atomic_store(&g_SharedData[idx].recvd_signals, 0);
    /* Clear local data */
    EnvData[idx].env = env;
    EnvData[idx].sigmask = 0;
    EnvData[idx].handler_vaddr = 0;
}

static int
sigd_invoke_handler(envid_t env, uint8_t sig_no) {
    /* TODO: implement */
    panic("Unimplemented");
}

static int sigd_serve_identify(envid_t from, const void* request,
                               void* response, int* response_perm);
static int sigd_serve_signal(envid_t from, const void* request,
                             void* response, int* response_perm);
static int sigd_serve_setprocmask(envid_t from, const void* request,
                                  void* response, int* response_perm);
static int sigd_serve_alarm(envid_t from, const void* request,
                            void* response, int* response_perm);
static int sigd_serve_set_handler(envid_t from, const void* request,
                                  void* response, int* response_perm);
static int sigd_serve_try_call_handler(envid_t from, const void* request,
                                       void* response, int* response_perm);

static union SigdRequest Request;
static union KmodIdentifyResponse Response;

struct RpcServer Server = {
        .ReceiveBuffer = &Request,
        .SendBuffer = &Response,
        .Fallback = NULL,
        .HandlerCount = SIGD_NREQUESTS,
        .Handlers = {
                [SIGD_REQ_IDENTIFY] = sigd_serve_identify,

                [SIGD_REQ_TRY_CALL_HANDLER_] = sigd_serve_try_call_handler,

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
sigd_serve_try_call_handler(envid_t from, const void* request,
                            void* response, int* response_perm) {
    if (from != LoopEnvid)
        return -E_BAD_ENV;

    int res = 0;
    const union SigdRequest* sigd_req = request;

    const size_t idx = sigd_req->try_call_handler_.target_idx;
    envid_t env_id = envs[idx].env_id;

    /* Reused env */
    /* NOTE: this snippet should probably go into all other RPC functions here */
    if (EnvData[idx].env != env_id) {
        sigd_reset_env_data(env_id);
        return 0;
    }

    /* TODO: Maybe it would be better to only modify lower bits of the status.
     *       In that case, SYS_env_exchange_status will need another parameter
     *       - status mask (only bits in mask should be modified) */
    int env_status = sys_env_exchange_status(env_id, ENV_NOT_RUNNABLE);

    /* Dead env */
    if (env_status < 0)
        return 0;

    if (!env_status_accepts_signals(env_status)) {
        res = sys_env_set_status(env_id, env_status);
        assert(res == 0);
        return 0;
    }

    const uint8_t sig_no = sigd_req->try_call_handler_.sig_no;
    uint64_t signals = atomic_fetch_and(
            &g_SharedData[idx].recvd_signals, ~(1ULL << sig_no));

    /* Signal was falsely sent after reset, ignore it */
    if (!(signals & (1ULL << sig_no))) {
        res = sys_env_set_status(env_id, env_status);
        assert(res == 0);
        return 0;
    }

    sigd_invoke_handler(env_id, sig_no);

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
        sigd_signal_loop(thisenv->env_id);
        panic("Unreachable code");
    }

    envid_t child = res;

    static_assert(sizeof(g_SharedData) % PAGE_SIZE == 0, "Unaligned shared data");
    res = sys_map_region(CURENVID, g_SharedData, child, g_SharedData,
                         sizeof(g_SharedData), PROT_RW | PROT_SHARE);
    if (res < 0) goto error;

    res = sys_env_set_status(child, ENV_RUNNABLE);
    if (res < 0) goto error;

    return child;
error:
    sys_env_destroy(child);
    return res;
}
