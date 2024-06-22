
#include <inc/lib.h>
#include <inc/rpc.h>
#include <inc/kmod/signal.h>


static void
notify_parent() {
    kill(thisenv->env_parent_id, SIGCHLD);
}

void
exit(void) {
    close_all();
    notify_parent();
    sys_env_destroy(0);
}
