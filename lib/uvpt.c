/* User virtual page table helpers */

#include "inc/types.h"
#include <inc/lib.h>
#include <inc/mmu.h>

extern volatile pte_t uvpt[];     /* VA of "virtual page table" */
extern volatile pde_t uvpd[];     /* VA of current page directory */
extern volatile pdpe_t uvpdp[];   /* VA of current page directory pointer */
extern volatile pml4e_t uvpml4[]; /* VA of current page map level 4 */

pte_t
get_uvpt_entry(void *va) {
    if (!(uvpml4[VPML4(va)] & PTE_P)) return uvpml4[VPML4(va)];
    if (!(uvpdp[VPDP(va)] & PTE_P) || (uvpdp[VPDP(va)] & PTE_PS)) return uvpdp[VPDP(va)];
    if (!(uvpd[VPD(va)] & PTE_P) || (uvpd[VPD(va)] & PTE_PS)) return uvpd[VPD(va)];
    return uvpt[VPT(va)];
}

uintptr_t
get_phys_addr(void *va) {
    if (!(uvpml4[VPML4(va)] & PTE_P))
        return -1;
    if (!(uvpdp[VPDP(va)] & PTE_P))
        return -1;
    if (uvpdp[VPDP(va)] & PTE_PS)
        return PTE_ADDR(uvpdp[VPDP(va)]) + ((uintptr_t)va & ((1ULL << PDP_SHIFT) - 1));
    if (!(uvpd[VPD(va)] & PTE_P))
        return -1;
    if ((uvpd[VPD(va)] & PTE_PS))
        return PTE_ADDR(uvpd[VPD(va)]) + ((uintptr_t)va & ((1ULL << PD_SHIFT) - 1));
    if (!(uvpt[VPT(va)] & PTE_P))
        return -1;
    return PTE_ADDR(uvpt[VPT(va)]) + PAGE_OFFSET(va);
}

int
get_prot(void *va) {
    pte_t pte = get_uvpt_entry(va);
    int prot = pte & PTE_AVAIL & ~PTE_SHARE;
    if (pte & PTE_P) prot |= PROT_R;
    if (pte & PTE_W) prot |= PROT_W;
    if (!(pte & PTE_NX)) prot |= PROT_X;
    if (pte & PTE_SHARE) prot |= PROT_SHARE;
    return prot;
}

bool
is_page_dirty(void *va) {
    pte_t pte = get_uvpt_entry(va);
    return pte & PTE_D;
}

bool
is_page_present(void *va) {
    return get_uvpt_entry(va) & PTE_P;
}

int
foreach_shared_region(int (*fun)(void *start, void *end, void *arg), void *arg) {
    /* Calls fun() for every shared region */

    int res = 0;
    for (size_t i = 0; i < MAX_USER_ADDRESS; i += PAGE_SIZE) {
        if (!(uvpml4[VPML4(i)] & PTE_P)) {
            i += HUGE_PAGE_SIZE * PD_ENTRY_COUNT * PDP_ENTRY_COUNT - PAGE_SIZE;
            continue;
        }
        if (!(uvpdp[VPDP(i)] & PTE_P)) {
            i += HUGE_PAGE_SIZE * PD_ENTRY_COUNT - PAGE_SIZE;
            continue;
        } else if (uvpdp[VPDP(i)] & PTE_PS) {
            if ((uvpdp[VPDP(i)] & (PTE_SHARE | PTE_P)) == (PTE_SHARE | PTE_P)) {
                res = fun((void *)i, (void *)(i + HUGE_PAGE_SIZE * PD_ENTRY_COUNT), arg);
                if (res < 0) break;
            }
            i += HUGE_PAGE_SIZE * PD_ENTRY_COUNT - PAGE_SIZE;
            continue;
        }
        if (!(uvpd[VPD(i)] & PTE_P)) {
            i += HUGE_PAGE_SIZE - PAGE_SIZE;
            continue;
        } else if (uvpd[VPD(i)] & PTE_PS) {
            if ((uvpd[VPD(i)] & (PTE_SHARE | PTE_P)) == (PTE_SHARE | PTE_P)) {
                res = fun((void *)i, (void *)(i + HUGE_PAGE_SIZE), arg);
                if (res < 0) break;
            }
            i += HUGE_PAGE_SIZE - PAGE_SIZE;
            continue;
        }
        if ((uvpt[VPT(i)] & (PTE_P | PTE_SHARE)) == (PTE_P | PTE_SHARE)) {
            res = fun((void *)i, (void *)(i + PAGE_SIZE), arg);
            if (res < 0) break;
        }
    }

    return res;
}

void force_alloc(void* va, size_t size) {
  volatile char* addr = va;
  for (size_t i = 0; i < size; i += PAGE_SIZE) {
    addr[i] = addr[i];
  }
}
