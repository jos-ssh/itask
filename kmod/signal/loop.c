#include <stdatomic.h>
#include <inc/assert.h>
#include <inc/lib.h>
#include "signal.h"

void
sigd_signal_loop(void) {
    cprintf("[%08x: sigd-loop] Starting up main loop...\n", thisenv->env_id);

    /* Shared page check */
    assert(atomic_load(&g_SharedData[ENVX(thisenv->env_id)].env) == thisenv->env_id);

    for (;;) {
        // TODO: implement signal loop
        sys_yield();
    }
}
