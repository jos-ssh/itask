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

#include "inc/kmod/request.h"

#ifndef INITD_VERSION
#define INITD_VERSION 0
#endif // !INITD_VERSION

#define INITD_MODNAME "jos.core.init"

enum InitdRequestType {
  INITD_REQ_IDENTIFY = KMOD_REQ_IDENTIFY,

  INITD_REQ_FIND_KMOD = KMOD_REQ_FIRST_USABLE,

  INITD_NREQUESTS
};

union InitdRequest {
  struct InitdFindKmod {
    ssize_t min_version;
    ssize_t max_version;
    char name_prefix[KMOD_MAXNAMELEN];
  } find_kmod;
  
  uint8_t pad_[PAGE_SIZE];
} __attribute__((aligned(PAGE_SIZE)));

#endif /* init.h */
