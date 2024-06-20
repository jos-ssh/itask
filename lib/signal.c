#include <inc/lib.h>


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
    // TODO:
    return -1;
}