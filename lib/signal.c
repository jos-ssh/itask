#include <inc/lib.h>
#include <inc/rpc.h>
#include <inc/kmod/signal.h>


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
    // FIXME: maybe bug with alarm(0)
    sys_env_set_status(CURENVID, ENV_NOT_RUNNABLE);
    return 0;
}