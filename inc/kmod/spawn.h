/**
 * @file spawn.h
 * @author MeerkatBoss (solodovnikov.ia@phystech.edu)
 *
 * @brief
 *
 * @version 0.1
 * @date 2024-06-09
 *
 * @copyright Copyright MeerkatBoss (c) 2024
 */
#ifndef __INC_KMOD_SPAWN_H
#define __INC_KMOD_SPAWN_H

#include <inc/assert.h>
#include <inc/fs.h>
#include <inc/kmod/request.h>
#include <inc/mmu.h>

#ifndef SPAWND_VERSION
#define SPAWND_VERSION 0
#endif // !SPAWND_VERSION

#define SPAWND_MODNAME "jos.core.spawn"

// 170 arguments of length 16 is maximum
#define SPAWND_MAXARGS 170
#define SPAWND_ARGLEN  16

enum SpawndRequestType {
    SPAWND_REQ_IDENTIFY = KMOD_REQ_IDENTIFY,

    SPAWND_REQ_FORK = KMOD_REQ_FIRST_USABLE,
    SPAWND_REQ_SPAWN,

    SPAWND_NREQUESTS
};

union SpawndRequest {
    struct NewEnv {
        // Path to executable file
        char file[MAXPATHLEN];
        // User id of loaded process (unused in user-mode)
        uint64_t uid;
        // Env type of loaded process (unused in user-mode)
        uint16_t env_type;
        // Number of arguments passed to process.
        // If 0, `argc` of process will be 1, and `argv[0]` will be equal to `file`
        uint16_t argc;
        // Argument vector. Strings are encoded as offsets into `strtab`
        uint16_t argv[SPAWND_MAXARGS];

        // Array of NUL-terminated strings
        char strtab[SPAWND_MAXARGS * SPAWND_ARGLEN];
    } env;

    uint8_t pad_[PAGE_SIZE];
} __attribute__((aligned(PAGE_SIZE), packed));

static_assert(sizeof(union SpawndRequest) == PAGE_SIZE, "spawnd request is too big");

#endif /* spawn.h */
