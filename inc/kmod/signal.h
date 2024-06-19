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
#ifndef __INC_KMOD_SIGNAL_H
#define __INC_KMOD_SIGNAL_H

#include <inc/kmod/request.h>
#include <inc/env.h>

#ifndef SIGD_VERSION
#define SIGD_VERSION 0
#endif // !SIGD_VERSION

#define SIGD_MODNAME "jos.ext.signal"

enum SigdRequestType {
    SIGD_REQ_IDENTIFY = KMOD_REQ_IDENTIFY,

    SIGD_REQ_SIGNAL = KMOD_REQ_FIRST_USABLE,
    SIGD_REQ_SETPROCMASK,
    SIGD_REQ_ALARM,
    SIGD_REQ_SET_HANDLER,

    /* internal requests, unavailable for users and other modules */
    SIGD_REQ_TRY_CALL_HANDLER_,

    SIGD_NREQUESTS
};

union SigdRequest {
    struct SigdSignal {
        envid_t target;
        uint8_t signal;
    } signal;
    struct SigdSetprocmask {
        envid_t target;
        uint64_t new_mask;
    } setprocmask;
    struct SigdAlarm {
        envid_t target;
        uint64_t time;
    } alarm;
    struct SigdSetHandler {
        envid_t target;
        uintptr_t handler_vaddr;
    } set_handler;

    /* Signal loop is user process and cannot modify trapframes, so this action
     * must be delegated to server */
    struct SigdTryCallHandler_ {
        size_t target_idx;
        uint8_t sig_no;
    } try_call_handler_;

    uint8_t pad_[PAGE_SIZE];
} __attribute__((aligned(PAGE_SIZE)));

#endif /* signal.h */
