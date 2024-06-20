#include <inc/lib.h>
#include <inc/rpc.h>
#include <inc/kmod/signal.h>


sighandler_t signal_handlers[NSIGNAL];


sighandler_t
signal(int sig_no, sighandler_t new_handler) {
    assert(sig_no < NSIGNAL);
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
    sys_env_set_status(CURENVID, ENV_NOT_RUNNABLE);
    return 0;
}