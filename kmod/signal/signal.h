/**
 * @file signal.h
 * @author MeerkatBoss (solodovnikov.ia@phystech.edu)
 *
 * @brief
 *
 * @version 0.1
 * @date 2024-06-19
 *
 * @copyright Copyright MeerkatBoss (c) 2024
 */
#ifndef __KMOD_SIGNAL_SIGNAL_H
#define __KMOD_SIGNAL_SIGNAL_H

#include <inc/types.h>
#include <inc/env.h>

struct SigdSharedData {
    _Atomic volatile uint64_t recvd_signals;
    _Atomic volatile uint64_t timer_countdown;
};

extern struct SigdSharedData g_SharedData[NENV];

#define ENV_IN_SIGHANDLER 0x8

static int
env_status_accepts_signals(int status) {
    return (status & (ENV_SCHED_STATUS_MASK | ENV_IN_SIGHANDLER)) == ENV_RUNNABLE;
}

void sigd_signal_loop(envid_t parent);

#endif /* signal.h */
