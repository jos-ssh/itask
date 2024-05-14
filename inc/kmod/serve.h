/**
 * @file serve.h
 * @author MeerkatBoss (solodovnikov.ia@phystech.edu)
 *
 * @brief
 *
 * @version 0.1
 * @date 2024-05-15
 *
 * @copyright Copyright MeerkatBoss (c) 2024
 */
#ifndef __INC_KMOD_SERVE_H
#define __INC_KMOD_SERVE_H

#include <inc/types.h>
#include <inc/env.h>

/**
 * @brief Kernel module RPC server handler
 */
typedef int kmod_serve_handler_t(envid_t from, void* request);

/**
 * @brief Listen for incoming IPC requests and call appropriate handlers
 */
void kmod_serve(void* buffer, size_t handler_count,
                kmod_serve_handler_t* const handlers[]);

#endif /* serve.h */
