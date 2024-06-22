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


#define signal_debug debug

struct SigdSharedData g_SharedData[NENV] __attribute__((aligned(PAGE_SIZE)));

static char SavedTrapframeBuf[PAGE_SIZE] __attribute((aligned(PAGE_SIZE)));

struct SigdEnvData {
    envid_t env;
    uint64_t sigmask;

    uintptr_t trapframe_vaddr;
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
sigd_invoke_handler(size_t env_idx, uint8_t sig_no) {
    struct Env* env = (struct Env*)&envs[env_idx];
    struct Trapframe tf_copy;
    memcpy(&tf_copy, &env->env_tf, sizeof(tf_copy));

    memcpy(SavedTrapframeBuf, &env->env_tf, sizeof(struct Trapframe));

    int res = sys_map_region(thisenv->env_id, SavedTrapframeBuf, env->env_id, (void*)EnvData[env_idx].trapframe_vaddr, PAGE_SIZE, PROT_RW);
    assert(res >= 0);

    // change rip to handler
    if (EnvData[env_idx].handler_vaddr == 0) {
        panic("handler vaddr doesn't set");
    }

    tf_copy.tf_rip = EnvData[env_idx].handler_vaddr;

    // change rdi to sig_no
    tf_copy.tf_regs.reg_rdi = sig_no;

    // change rsi to ptr to saved trapframe
    tf_copy.tf_regs.reg_rsi = EnvData[env_idx].trapframe_vaddr;

    // change trapframe
    sys_env_set_trapframe(env->env_id, &tf_copy);

    // change env status to ENV_RUNNABLE | ENV_IN_SIGHANDLER (to prevent recursive signals)
    sys_env_set_status(envs[env_idx].env_id, ENV_RUNNABLE | ENV_IN_SIGHANDLER);
    return 0;
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
                [SIGD_REQ_SIGNAL] = sigd_serve_signal,
                [SIGD_REQ_SETPROCMASK] = sigd_serve_setprocmask,
                [SIGD_REQ_ALARM] = sigd_serve_alarm,
                [SIGD_REQ_SET_HANDLER] = sigd_serve_set_handler,

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


static inline int
is_ignored_signal(uint64_t sigmask, uint8_t signal) {
    return (sigmask & ~(1ULL << signal));
}

static int
sigd_serve_signal(envid_t from, const void* request,
                  void* response, int* response_perm) {

    const union SigdRequest* sigd_req = request;
    const uint8_t signal = sigd_req->signal.signal;
    const envid_t env_id = sigd_req->signal.target;

    if (signal_debug) {
        cprintf("[sigd] setting up signal %d to env %d\n", signal, env_id);
    }

    size_t idx = ENVX(env_id);
    if (EnvData[idx].env != env_id) {
        if (signal_debug) {
            cprintf("[sigd] reset\n");
        }
        sigd_reset_env_data(env_id);
    }

    if (EnvData[idx].handler_vaddr == 0) {
        if (signal_debug) {
            cprintf("[sigd] empty handler\n");
        }
        return -E_INVAL;
    }

    if (is_ignored_signal(EnvData[idx].sigmask, signal)) {
        if (signal_debug) {
            cprintf("[sigd] signal %d ignored\n", signal);
        }
        return -E_INVAL;
    }

    // set signal to recvd mask
    atomic_fetch_or(
            &g_SharedData[idx].recvd_signals, 1ULL << signal);

    if (signal_debug) {
        cprintf("[sigd] recieved signals %lx\n", atomic_load(&g_SharedData[idx].recvd_signals));
    }
    return 0;
}


static int
sigd_serve_setprocmask(envid_t from, const void* request,
                       void* response, int* response_perm) {
    const union SigdRequest* sigd_req = request;
    const envid_t env_id = sigd_req->setprocmask.target;
    if (env_id != from) {
        return -E_BAD_ENV;
    }

    size_t idx = ENVX(env_id);
    if (EnvData[idx].env != env_id) {
        sigd_reset_env_data(env_id);
    }
    EnvData[idx].sigmask = sigd_req->setprocmask.new_mask;

    return 0;
}


static int
sigd_serve_alarm(envid_t from, const void* request,
                 void* response, int* response_perm) {
    const union SigdRequest* sigd_req = request;
    const envid_t env_id = sigd_req->alarm.target;
    if (env_id != from) {
        return -E_BAD_ENV;
    }


    if (signal_debug) {
        cprintf("[sigd] serve alarm from %d\n", env_id);
    }

    size_t idx = ENVX(env_id);
    if (EnvData[idx].env != env_id) {
        sigd_reset_env_data(env_id);
        return 0;
    }

    if (EnvData[idx].handler_vaddr == 0) {
        if (signal_debug) {
            cprintf("[sigd] no handler for alarm\n");
        }
        return -E_INVAL;
    }

    if (is_ignored_signal(EnvData[idx].sigmask, SIGALRM)) {
        if (signal_debug) {
            cprintf("[sigd] SIGALRM ignored\n");
        }
        return -E_INVAL;
    }
    atomic_store(&g_SharedData[idx].timer_countdown, sigd_req->alarm.time);
    return 0;
}


static int
sigd_serve_set_handler(envid_t from, const void* request,
                       void* response, int* response_perm) {
    const union SigdRequest* sigd_req = request;
    const envid_t env_id = sigd_req->set_handler.target;


    if (signal_debug) {
        cprintf("[sigd] setting up handler with vaddr 0x%08lx for env %d\n", sigd_req->set_handler.handler_vaddr, sigd_req->set_handler.target);
    }

    if (env_id != from) {
        return -E_BAD_ENV;
    }

    size_t idx = ENVX(env_id);
    if (EnvData[idx].env != env_id) {
        if (signal_debug) {
            cprintf("[sigd] reset data\n");
        }
        sigd_reset_env_data(env_id);
    }

    EnvData[idx].handler_vaddr = sigd_req->set_handler.handler_vaddr;
    EnvData[idx].trapframe_vaddr = sigd_req->set_handler.trapframe_vaddr;

    return 0;
}

static int
sigd_serve_try_call_handler(envid_t from, const void* request,
                            void* response, int* response_perm) {
    if (from != LoopEnvid) {
        if (signal_debug) {
            cprintf("[sigd] bad env\n");
        }
        return -E_BAD_ENV;
    }

    int res = 0;
    const union SigdRequest* sigd_req = request;

    const size_t idx = sigd_req->try_call_handler_.target_idx;
    envid_t env_id = envs[idx].env_id;

    if (signal_debug) {
        cprintf("[sigd] try call handler for signal %d\n", sigd_req->try_call_handler_.sig_no);
    }

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

    sigd_invoke_handler(idx, sig_no);

    return 0;
}

static int
sigd_start_loop(void) {
    static union InitdRequest initd_req;
    void* res_data = NULL;

    initd_req.fork.parent = CURENVID;
    envid_t parent = thisenv->env_id;
    rpc_execute(InitdEnvid, INITD_REQ_FORK, &initd_req, &res_data);
    thisenv = &envs[ENVX(sys_getenvid())];

    int res = thisenv->env_ipc_value;
    if (res < 0) return res;

    if (res == 0) {
        sigd_signal_loop(parent);
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
