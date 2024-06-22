#include "inc/env.h"
#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/env.h>
#include <kern/monitor.h>


struct Taskstate cpu_ts;
_Noreturn void sched_halt(void);

/* Choose a user environment to run and run it */
_Noreturn void
sched_yield(void) {
    /* Implement simple round-robin scheduling.
     *
     * Search through 'envs' for an ENV_RUNNABLE environment in
     * circular fashion starting just after the env was
     * last running.  Switch to the first such environment found.
     *
     * If no envs are runnable, but the environment previously
     * running is still ENV_RUNNING, it's okay to
     * choose that environment.
     *
     * If there are no runnable environments,
     * simply drop through to the code
     * below to halt the cpu */

    size_t start_idx = curenv ? (size_t)(curenv - envs) : 0;

    for (size_t i = 0; i < NENV; ++i) {
        size_t env_idx = (start_idx + i) % NENV;

        if (envs + env_idx == curenv) /* Skip current task while searhing */
            continue;

        if (env_check_sched_status(envs[env_idx].env_status, ENV_RUNNABLE)) {
            env_run(envs + env_idx);
        }
    }

    if (curenv && env_check_sched_status(curenv->env_status, ENV_RUNNING)) {
        env_run(curenv);
    }

    cprintf("Halt\n");
    curenv = NULL;

    /* No runnable environments,
     * so just halt the cpu */
    sched_halt();
}

/* Halt this CPU when there is nothing to do. Wait until the
 * timer interrupt wakes it up. This function never returns */
_Noreturn void
sched_halt(void) {

    /* For debugging and testing purposes, if there are no runnable
     * environments in the system, then drop into the kernel monitor */
    int i;
    for (i = 0; i < NENV; i++)
        if (env_check_sched_status(envs[i].env_status, ENV_RUNNABLE) ||
            env_check_sched_status(envs[i].env_status, ENV_RUNNING)) break;
    if (i == NENV) {
        cprintf("No runnable environments in the system!\n");
        for (;;) monitor(NULL);
    }

    /* Mark that no environment is running on CPU */
    curenv = NULL;

    /* Reset stack pointer, enable interrupts and then halt */
    asm volatile(
            "movq $0, %%rbp\n"
            "movq %0, %%rsp\n"
            "pushq $0\n"
            "pushq $0\n"
            "sti\n"
            "hlt\n" ::"a"(cpu_ts.ts_rsp0));

    /* Unreachable */
    for (;;);
}
