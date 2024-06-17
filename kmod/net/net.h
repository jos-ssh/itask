#pragma once

#include <inc/env.h>

extern envid_t g_InitdEnvid;
extern envid_t g_PcidEnvid;
extern bool g_IsNetdInitialized;

extern struct virtio_net_device_t net;

#define RECEIVE_ADDR 0x0FFFF000

#define VENDOR_ID 0x1AF4
#define DEVICE_ID 0x1000

#define UNWRAP(res, line) do { if (res != 0) { panic(line ": %i", res); } } while (0)

void initialize();

void serve_teapot();