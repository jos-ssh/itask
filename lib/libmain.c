/* Called from entry.S to get us going.
 * entry.S already took care of defining envs, pages, uvpd, and uvpt */

#include <inc/rpc.h>
#include <inc/kmod/signal.h>
#include <inc/lib.h>
#include <inc/x86.h>
#include <inc/unistd.h>

extern void umain(int argc, char **argv);

const volatile struct Env *thisenv;
const char *binaryname = "<unknown>";

char **environ;

// FIXME:
static char SavedTrapframeBuf[PAGE_SIZE] __attribute__((aligned(PAGE_SIZE)));
extern sighandler_t signal_handlers[NSIGNAL];

#ifdef JOS_PROG
void (*volatile sys_exit)(void);
#endif


static void
global_signal_handler(uint8_t sig_no, struct Trapframe *saved_tf) {
    assert(sig_no < NSIGNAL);

    sighandler_t handler = signal_handlers[sig_no];
    if (!handler) {
        panic("no handler for signal %d\n", sig_no);
    }
    handler(sig_no);

    sys_env_set_status(CURENVID, ENV_RUNNABLE);
    sys_env_set_trapframe(CURENVID, saved_tf);
    panic("Unreachable");
}


void
libmain(int argc, char **argv) {
    /* Perform global constructor initialisation (e.g. asan)
     * This must be done as early as possible */
    extern void (*__ctors_start)(), (*__ctors_end)();
    void (**ctor)() = &__ctors_start;
    while (ctor < &__ctors_end) (*ctor++)();

    /* Save the name of the program so that panic() can use it */
    if (argc > 0) binaryname = argv[0];

    /* Set thisenv to point at our Env structure in envs[]. */
    static_assert(UENVS_SIZE / sizeof(*envs), "Not ehough space for envs");
    const size_t env_count = UENVS_SIZE / sizeof(*envs);

    envid_t envid = sys_getenvid();
    const volatile struct Env *env = NULL;
    for (size_t i = 0; i < env_count; ++i) {
        if (envs[i].env_id == envid) {
            env = envs + i;
            break;
        }
    }
    if (!env) {
        panic("Env not available");
    }
    thisenv = env;

    if (thisenv->env_type == ENV_TYPE_USER) {
        envid_t sigdEnv = kmod_find_any_version(SIGD_MODNAME);

        static union SigdRequest request;
        request.set_handler.handler_vaddr = (uintptr_t)global_signal_handler;
        request.set_handler.trapframe_vaddr = (uintptr_t)SavedTrapframeBuf;
        request.set_handler.target = envid;

        void *res_data = NULL;

        rpc_execute(sigdEnv, SIGD_REQ_SET_HANDLER, &request, &res_data);

        signal(SIGKILL, _exit);
        signal(SIGTERM, _exit);
        signal(SIGALRM, _exit);
        signal(SIGCHLD, SIG_IGN);

        // TODO: add signal logic in pipe.c
        signal(SIGPIPE, SIG_IGN);
    }

    /* Call user main routine */
    umain(argc, argv);

#ifdef JOS_PROG
    sys_exit();
#else
    exit();
#endif
}
