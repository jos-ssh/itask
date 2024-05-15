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
enum KmodInitRequestType {
  KINIT_REQ_IDENTIFY = KMOD_REQ_IDENTIFY,

  KINIT_REQ_READ_ACPI = KMOD_REQ_FIRST_USABLE
};

struct KinitReadAcpiRequest {
  char Signature[4];
  size_t Offset;
};

#endif /* init.h */
