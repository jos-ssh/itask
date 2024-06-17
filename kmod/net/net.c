#include "net.h"
#include <inc/kmod/init.h>
#include <fs/pci_classes.h>
#include <kmod/pci/pci.h>
#include "queue.h"
/* #### Globals ####
*/

envid_t g_InitdEnvid;
envid_t g_PcidEnvid;
bool g_IsNetdInitialized = false;
struct virtio_net_device_t net;

/* #### I/O RPC
*/

void serve_teapot() {
    if (!g_IsNetdInitialized) {
        initialize();
    }
}
