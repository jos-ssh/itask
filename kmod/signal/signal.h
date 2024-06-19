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
    _Atomic volatile envid_t env;
    _Atomic volatile uint64_t recv_signals;
    _Atomic volatile uint64_t sigmask;
    _Atomic volatile uint64_t alarm_countdown;

    _Atomic volatile uintptr_t handler_vaddr;
};

extern struct SigdSharedData g_SharedData[NENV];

void sigd_signal_loop(void);

#endif /* signal.h */
