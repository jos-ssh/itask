#include "inc/lib.h"
#include "signal.h"

void
sigd_signal_loop(void) {
    // TODO: implement signal loop
    for (;;) {
        sys_yield();
    }
}
