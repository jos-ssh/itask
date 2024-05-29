#include "mmio.h"

#include <inc/lib.h>
#include <inc/memlayout.h>
#include <inc/mmu.h>
#include <inc/types.h>

#define MMIO_MAX_HEAP_SIZE HUGE_PAGE_SIZE

static char* MmioHeapTop = (char*)UTEMP;
static size_t MmioHeapSize = 0;

static void* LastRegion = NULL;
static size_t LastSize = 0;

void*
mmio_map_region(physaddr_t paddr, size_t size) {
    uintptr_t start = ROUNDDOWN(paddr, PAGE_SIZE);
    uintptr_t end = ROUNDUP(paddr + size, PAGE_SIZE);
    size_t true_size = end - start;
    
    if (MmioHeapSize + true_size >= MMIO_MAX_HEAP_SIZE) {
        cprintf("[%08x: pcid] Out of MMIO memory!\n", thisenv->env_id);
        return NULL;
    }

    int res = sys_map_physical_region(start, CURENVID, MmioHeapTop, true_size,
                                      PROT_R | PROT_CD);
    assert(res >= 0);

    LastRegion = MmioHeapTop + (paddr - start);
    LastSize = size;

    // Commit allocation
    MmioHeapTop += true_size;
    MmioHeapSize += true_size;

    return LastRegion;
}

void*
mmio_remap_last_region(physaddr_t paddr, void* old_vaddr,
                       size_t old_size, size_t new_size) {
    assert(old_vaddr == LastRegion);
    assert(old_size == LastSize);

    uintptr_t start = ROUNDDOWN(paddr, PAGE_SIZE);
    uintptr_t old_end = ROUNDUP(paddr + old_size, PAGE_SIZE);
    uintptr_t new_end = ROUNDUP(paddr + new_size, PAGE_SIZE);

    size_t old_true_size = old_end - start;
    size_t new_true_size = new_end - start;

    if (new_true_size <= old_true_size) {
        LastSize = new_size;
        return LastRegion;
    }

    size_t size_diff = new_true_size - old_true_size;
    if (MmioHeapSize + size_diff >= MMIO_MAX_HEAP_SIZE) {
        cprintf("[%08x: pcid] Out of MMIO memory!\n", thisenv->env_id);
        return NULL;
    }

    int res = sys_map_physical_region(old_end, CURENVID, MmioHeapTop,
                                      size_diff, PROT_R | PROT_CD);
    assert(res >= 0);

    LastSize = new_size;

    // Commit allocation
    MmioHeapTop += size_diff;
    MmioHeapSize += size_diff;

    return LastRegion;
}
