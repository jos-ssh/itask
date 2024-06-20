#include <inc/lib.h>
#include <inc/kmod/signal.h>
#include <inc/rpc.h>


void
self_kill_test() {
    envid_t sigdEnv = kmod_find_any_version(SIGD_MODNAME);

    static union SigdRequest request;

    request.signal.signal = SIGKILL;
    request.signal.target = thisenv->env_id;

    void *res_data = NULL;

    rpc_execute(sigdEnv, SIGD_REQ_SIGNAL, &request, &res_data);
    sys_yield();
}


void
sigalarm_handler(int sig_no) {
    printf("sigalrm catched\n");
}

void
custom_handler_test() {
    signal(SIGALRM, sigalarm_handler);
    alarm(1);
}

void
umain(int argc, char **argv) {
    printf("Test 1: alarm\n");
    alarm(5);
    printf("5 seconds later\n");

    printf("Test 2: signal\n");
    custom_handler_test();

    printf("Test 3: killing myself...\n");
    self_kill_test();
    printf("i am alive :(");
}