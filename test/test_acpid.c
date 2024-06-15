/* hello, world */
#include "inc/env.h"
#include "inc/rpc.h"
#include "inc/stdio.h"
#include <inc/lib.h>
#include <inc/kmod/request.h>
#include <inc/acpi.h>
#include <inc/kmod/acpi.h>

#define RECEIVE_ADDR 0x0FFFF000

typedef struct {
    uint64_t BaseAddress;
    uint16_t SegmentGroup;
    uint8_t StartBus;
    uint8_t EndBus;
    uint32_t Reserved;
} CSBAA;

typedef struct {
    ACPISDTHeader h;
    uint64_t Reserved;
    CSBAA Data[];
} MCFG;

MCFG*
get_mcfg(envid_t acpid) {
    static union AcpidRequest req;

    strncpy(req.find_table.Signature, "MCFG", 4);
    req.find_table.Offset = 0;

    union AcpidResponse* res = (void*)RECEIVE_ADDR;
    int status = rpc_execute(acpid, ACPID_REQ_FIND_TABLE, &req, (void**)&res);
    if (status < 0) {
        panic("Failed to read MCFG: %i\n", status);
    }
    assert(status == res->table_start.Header.Length);

    return (MCFG*)(res);
}

void
umain(int argc, char** argv) {
    envid_t acpid = kmod_find_any_version(ACPID_MODNAME);
    cprintf("Found 'acpid' in env [%08x]\n", acpid);

    MCFG* mcfg = get_mcfg(acpid);
    cprintf("MCFG checksum=%04x\n", (uint32_t)mcfg->h.Checksum);
}
