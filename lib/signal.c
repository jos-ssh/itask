#include <inc/signal.h>
#include <inc/unistd.h>

#include <inc/rpc.h>
#include <inc/kmod.h>
#include <inc/kmod/signal.h>
#include <inc/kmod/init.h>
#include <inc/lib.h>

sighandler_t signal_handlers[NSIGNAL];

static void
default_handler(int sig_no) {
    exit();
}

static void
noop_handler(int sig_no) {
    // No-op
}

sighandler_t
signal(int sig_no, sighandler_t new_handler) {
    assert(sig_no < NSIGNAL);

    if (new_handler == SIG_DFL) {
        new_handler = default_handler;
    } else if (new_handler == SIG_IGN) {
        new_handler = noop_handler;
    }

    sighandler_t old_handler = signal_handlers[sig_no];
    signal_handlers[sig_no] = new_handler;
    return old_handler;
}

unsigned int
alarm(unsigned int seconds) {
    envid_t sigdEnv = kmod_find_any_version(SIGD_MODNAME);

    static union SigdRequest request;
    request.alarm.time = seconds;
    request.alarm.target = thisenv->env_id;

    void *res_data = NULL;

    rpc_execute(sigdEnv, SIGD_REQ_ALARM, &request, &res_data);
    return 0;
}

int
kill(envid_t env, int sig_no) {
    if (!env || env == kmod_find_any_version(INITD_MODNAME)) {
        return -E_INVAL;
    }

    envid_t sigdEnv = kmod_find_any_version(SIGD_MODNAME);

    static union SigdRequest request;
    request.signal.target = env;
    request.signal.signal = sig_no;

    void *res_data = NULL;

    rpc_execute(sigdEnv, SIGD_REQ_SIGNAL, &request, &res_data);
    return 0;
}