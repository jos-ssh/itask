#include <stdatomic.h>
#include <inc/assert.h>
#include <inc/lib.h>
#include "inc/env.h"
#include "inc/kmod/signal.h"
#include "inc/rpc.h"
#include "signal.h"

static int
get_any_signal(uint64_t signals) {
    for (size_t i = 0; i < 64; ++i)
        if (signals & (1UL << i))
            return i;
    return -1;
}

static void update_timers();
static void send_signals(envid_t server);

void
sigd_signal_loop(envid_t parent) {
    cprintf("[%08x: sigd-loop] Starting up main loop...\n", thisenv->env_id);

    for (;;) {
        update_timers();
        send_signals(parent);
        sys_yield();
    }
}

static void
send_signals(envid_t server) {
    static union SigdRequest req;
    for (size_t i = 0; i < NENV; ++i) {
        if (i == ENVX(thisenv->env_id) || i == ENVX(server))
            continue;

        uint64_t sigmask = atomic_load(&g_SharedData[i].recvd_signals);
        int sig = get_any_signal(sigmask);
        if (sig < 0)
            continue;

        int status = envs[i].env_status;
        if (!env_status_accepts_signals(status))
            continue;

        req.try_call_handler_.target_idx = i;
        req.try_call_handler_.sig_no = sig;

        void* res_data = NULL;
        rpc_execute(server, SIGD_REQ_TRY_CALL_HANDLER_, &req, &res_data);
    }
}

static void
update_timers() {
    /* TODO: implement (decrement timers and set SIGALRM flag in recvd_signals */
}
