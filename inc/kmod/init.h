/**
 * @file init.h
 * @author MeerkatBoss (solodovnikov.ia@phystech.edu)
 *
 * @brief
 *
 * @version 0.1
 * @date 2024-05-15
 *
 * @copyright Copyright MeerkatBoss (c) 2024
 */
#ifndef __INC_KMOD_INIT_H
#define __INC_KMOD_INIT_H

#include <inc/assert.h>
#include <inc/kmod/request.h>
#include <inc/fs.h>

#ifndef INITD_VERSION
#define INITD_VERSION 0
#endif // !INITD_VERSION

#define INITD_MODNAME "jos.core.init"

// 170 arguments of length 16 is maximum
#define SPAWN_MAXARGS 170
#define SPAWN_ARGLEN  16

enum InitdRequestType {
  INITD_REQ_IDENTIFY = KMOD_REQ_IDENTIFY,

  INITD_REQ_FIND_KMOD = KMOD_REQ_FIRST_USABLE,
  INITD_REQ_FORK,
  INITD_REQ_SPAWN,

  INITD_NREQUESTS
};

union InitdRequest {
  struct InitdFindKmod {
    ssize_t min_version;
    ssize_t max_version;
    char name_prefix[KMOD_MAXNAMELEN];
  } find_kmod;

  struct InitdSpawn {
      // Path to executable file
      char file[MAXPATHLEN];
      // RUID of loaded process (unused in user-mode)
      uint64_t ruid;
      // Number of arguments passed to process.
      // If 0, `argc` of process will be 1, and `argv[0]` will be equal to `file`
      uint16_t argc;
      // Argument vector. Strings are encoded as offsets into `strtab`
      uint16_t argv[SPAWN_MAXARGS];

      // Array of NUL-terminated strings
      char strtab[SPAWN_MAXARGS * SPAWN_ARGLEN];
  } spawn;
  
  uint8_t pad_[PAGE_SIZE];
} __attribute__((aligned(PAGE_SIZE)));

static_assert(sizeof(union InitdRequest) == PAGE_SIZE, "initd request is too big");

#endif /* init.h */
