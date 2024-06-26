#pragma once

#include <inc/error.h>

extern int __jos_errno_loc;

#define errno __jos_errno_loc