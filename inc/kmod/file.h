/**
 * @file file.c
 * @author MeerkatBoss (solodovnikov.ia@phystech.edu)
 *
 * @brief
 *
 * @version 0.1
 * @date 2024-06-17
 *
 * @copyright Copyright MeerkatBoss (c) 2024
 */
#ifndef __INC_KMOD_FILE_C
#define __INC_KMOD_FILE_C

#include <inc/kmod/request.h>
#include <inc/kmod/init.h>
#include <inc/fs.h>

#ifndef FILED_VERSION
#define FILED_VERSION 0
#endif // !FILED_VERSION

#define FILED_MODNAME "jos.core.fs_wrapper"

enum FiledRequestType {
    FILED_REQ_IDENTIFY = KMOD_REQ_IDENTIFY,

    FILED_REQ_OPEN = KMOD_REQ_FIRST_USABLE,
    FILED_REQ_SPAWN,
    FILED_REQ_FORK,
    FILED_REQ_REMOVE,
    FILED_REQ_CHMOD,
    FILED_REQ_CHOWN,
    FILED_REQ_GETCWD,
    FILED_REQ_SETCWD,

    FILED_NREQUESTS
};

union FiledRequest {
    struct FiledOpen {
        char req_path[MAXPATHLEN];
        int req_omode;
        int req_flags;
        uintptr_t req_fd_vaddr;
    } open;
    struct FiledSpawn {
        char req_path[MAXPATHLEN];
        uint16_t req_argc;
        uint16_t req_argv[SPAWN_MAXARGS];
        char req_strtab[SPAWN_MAXARGS * SPAWN_ARGLEN];
    } spawn;
    struct FiledRemove {
        char req_path[MAXPATHLEN];
    } remove;
    struct FiledChmod {
        char req_path[MAXPATHLEN];
        uint32_t req_mode;
    } chmod;
    struct FiledChown {
        char req_path[MAXPATHLEN];
        uint64_t req_uid;
        uint64_t req_gid;
    } chown;
    struct FiledSetcwd {
        char req_path[MAXPATHLEN];
    } setcwd;
    struct FiledGetcwd {
    } getcwd;

    uint8_t pad_[PAGE_SIZE];
} __attribute((aligned(PAGE_SIZE)));

union FiledResponse {
    char cwd[MAXPATHLEN];

    uint8_t pad_[PAGE_SIZE];
} __attribute__((aligned(PAGE_SIZE)));


#endif /* file.c */
