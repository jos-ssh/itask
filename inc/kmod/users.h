#pragma once

#include <inc/kmod/request.h>

#define USERSD_VERSION 0
#define USERSD_MODNAME "jos.core.users"

enum UsersdRequestType {
  USERSD_REQ_IDENTIFY = KMOD_REQ_IDENTIFY,

  USERSD_REQ_LOGIN = KMOD_REQ_FIRST_USABLE,

  USERSD_NREQUESTS
};

union UsersdRequest {

  uint8_t pad_[PAGE_SIZE];
} __attribute__((aligned(PAGE_SIZE)));

union UsersdResponse {

  uint8_t pad_[PAGE_SIZE];
} __attribute__((aligned(PAGE_SIZE)));
