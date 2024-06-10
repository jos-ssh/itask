#pragma once

#include <inc/env.h>

extern envid_t g_InitdEnvid;
extern envid_t g_PcidEnvid;
extern bool g_IsNetdInitialized;

#define RECEIVE_ADDR 0x0FFFF000


void serve_teapot();