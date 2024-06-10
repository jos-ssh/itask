#include "net.h"
#include "inc/kmod/pci.h"
#include "inc/stdio.h"
#include <inc/lib.h>
#include <inc/rpc.h>
#include <inc/kmod/init.h>

/* #### Globals ####
*/

envid_t g_InitdEnvid;
envid_t g_PcidEnvid;
bool g_IsNetdInitialized = false;

#define VENDOR_ID 0x1AF4
#define DEVICE_ID 0x1000
#define SUBSYSTEM_ID 0x01

/* ### Static declarations ####
*/

static envid_t
find_module(envid_t initd, const char* name);

/* #### Initialization ####
*/

void initialize() {
    g_PcidEnvid = find_module(g_InitdEnvid, PCID_MODNAME);

    // __auto_type device_id = pcid_device_id(); 

    union PcidResponse* response = (void*) RECEIVE_ADDR;
    rpc_execute(g_PcidEnvid, PCID_REQ_FIND_DEVICE, NULL, (void **)&response);

    g_IsNetdInitialized = true;
} 

/* #### Helper functions ####
*/

static envid_t
find_module(envid_t initd, const char* name) {
    static union InitdRequest request;

    request.find_kmod.max_version = -1;
    request.find_kmod.min_version = -1;
    strcpy(request.find_kmod.name_prefix, name);

    void* res_data = NULL;
    return rpc_execute(initd, INITD_REQ_FIND_KMOD, &request, &res_data);
}