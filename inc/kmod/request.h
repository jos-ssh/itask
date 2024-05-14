/**
 * @file request.h
 * @author MeerkatBoss (solodovnikov.ia@phystech.edu)
 *
 * @brief
 *
 * @version 0.1
 * @date 2024-05-15
 *
 * @copyright Copyright MeerkatBoss (c) 2024
 */
#ifndef __INC_KMOD_REQUEST_H
#define __INC_KMOD_REQUEST_H

#include <inc/mmu.h>

/**
 * @brief RPC ID for request
 */
enum KernModuleRequestType {
  KMOD_REQ_IDENTIFY = 0,      // Get module name
  
  KMOD_REQ_FIRST_USABLE = 16  // First non-reserved id
};

enum {
  KMOD_MAXNAMELEN = 1024
};

/**
 * @brief Identification information about kernel module
 */
struct KmodInfo {
  size_t version;
  char name[KMOD_MAXNAMELEN];
};

/**
 * @brief IDENTIFY response body
 */
struct KmodIdentifyResponse {
  struct KmodInfo info;
  char rzvd__[PAGE_SIZE - sizeof(struct KmodInfo)];
} __attribute__((packed));

#endif /* request.h */
