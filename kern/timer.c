#include <inc/assert.h>
#include <inc/error.h>
#include <inc/memlayout.h>
#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/trap.h>
#include <inc/types.h>
#include <inc/uefi.h>
#include <inc/x86.h>

#include <kern/kclock.h>
#include <kern/picirq.h>
#include <kern/pmap.h>
#include <kern/sched.h>
#include <kern/timer.h>
#include <kern/trap.h>
#include <kern/tsc.h>

#define RSDP_FIRST_BYTES        20
#define ACPI_VERSION_1_0        0
#define ACPI_VERSION_2_0_higher 2
#define ACPI_SIGN_LENGTH        4

#define kilo      (1000ULL)
#define Mega      (kilo * kilo)
#define Giga      (kilo * Mega)
#define Tera      (kilo * Giga)
#define Peta      (kilo * Tera)
#define ULONG_MAX ~0UL

#if LAB <= 6
/* Early variant of memory mapping that does 1:1 aligned area mapping
 * in 2MB pages. You will need to reimplement this code with proper
 * virtual memory mapping in the future. */
void *
mmio_map_region(physaddr_t pa, size_t size) {
    void map_addr_early_boot(uintptr_t addr, uintptr_t addr_phys, size_t sz);
    const physaddr_t base_2mb = 0x200000;
    uintptr_t org = pa;
    size += pa & (base_2mb - 1);
    size += (base_2mb - 1);
    pa &= ~(base_2mb - 1);
    size &= ~(base_2mb - 1);
    map_addr_early_boot(pa, pa, size);
    return (void *)org;
}
void *
mmio_remap_last_region(physaddr_t pa, void *addr, size_t oldsz, size_t newsz) {
    return mmio_map_region(pa, newsz);
}
#endif

struct Timer timertab[MAX_TIMERS];
struct Timer *timer_for_schedule;

struct Timer timer_hpet0 = {
        .timer_name = "hpet0",
        .timer_init = hpet_init,
        .get_cpu_freq = hpet_cpu_frequency,
        .enable_interrupts = hpet_enable_interrupts_tim0,
        .handle_interrupts = hpet_handle_interrupts_tim0,
};

struct Timer timer_hpet1 = {
        .timer_name = "hpet1",
        .timer_init = hpet_init,
        .get_cpu_freq = hpet_cpu_frequency,
        .enable_interrupts = hpet_enable_interrupts_tim1,
        .handle_interrupts = hpet_handle_interrupts_tim1,
};

struct Timer timer_acpipm = {
        .timer_name = "pm",
        .timer_init = acpi_enable,
        .get_cpu_freq = pmtimer_cpu_frequency,
};

void
acpi_enable(void) {
    FADT *fadt = get_fadt();
    outb(fadt->SMI_CommandPort, fadt->AcpiEnable);
    while ((inw(fadt->PM1aControlBlock) & 1) == 0) /* nothing */
        ;
}

static int
check_parity(const void *data, unsigned long data_size) {
    unsigned char sum = 0;
    for (size_t i = 0; i < data_size; ++i) {
        sum += ((const char *)data)[i];
    }
    return sum == 0;
}

static int
try_get_acpi_table(uint64_t phys_addr, void **table) {
    ACPISDTHeader *table_header = mmio_map_region(phys_addr, sizeof(ACPISDTHeader));
    table_header = mmio_remap_last_region(
            phys_addr,
            table_header,
            sizeof(ACPISDTHeader),
            table_header->Length);

    if (!check_parity(table_header, table_header->Length)) {
        return -E_ACPI_BAD_CHECKSUM;
    }

    *table = (void *)table_header;
    return 0;
}

static void *
acpi_find_table_xsdt(const char *sign, const XSDT *xsdt) {
    const size_t xsdt_full_size = xsdt->h.Length;
    if (xsdt_full_size < sizeof(XSDT)) {
        panic("XSDT is too small");
    }

    const size_t xsdt_content_size = xsdt_full_size - sizeof(XSDT);
    if (xsdt_content_size & 0x7) {
        panic("XSDT content size is not a multiple of 8");
    }

    const size_t xsdt_entry_count = xsdt_content_size >> 3;
    for (size_t i = 0; i < xsdt_entry_count; ++i) {
        ACPISDTHeader *table = NULL;
        int status = try_get_acpi_table(xsdt->PointerToOtherSDT[i], (void **)&table);
        if (status < 0) {
            panic("Error getting ACPI table: %i", status);
        }

        if (strncmp(sign, table->Signature, ACPI_SIGN_LENGTH) == 0) {
            return table;
        }
    }

    return NULL;
}

static void *
acpi_find_table_rsdt(const char *sign, const RSDT *rsdt) {
    const size_t rsdt_full_size = rsdt->h.Length;
    if (rsdt_full_size < sizeof(RSDT)) {
        panic("RSDT is too small");
    }

    const size_t rsdt_content_size = rsdt_full_size - sizeof(RSDT);
    if (rsdt_content_size & 0x3) {
        panic("RSDT content size is not a multiple of 4");
    }

    const size_t rsdt_entry_count = rsdt_content_size >> 3;
    for (size_t i = 0; i < rsdt_entry_count; ++i) {
        ACPISDTHeader *table = NULL;
        int status = try_get_acpi_table(rsdt->PointerToOtherSDT[i], (void **)&table);
        if (status < 0) {
            panic("Error getting ACPI table: %i", status);
        }

        if (strncmp(sign, table->Signature, ACPI_SIGN_LENGTH) == 0) {
            return table;
        }
    }

    return NULL;
}

RSDP *
get_rsdp(void) {
    return mmio_map_region(uefi_lp->ACPIRoot, sizeof(RSDP));
}

static void *
acpi_find_table(const char *sign) {
    /*
     * This function performs lookup of ACPI table by its signature
     * and returns valid pointer to the table mapped somewhere.
     *
     * It is a good idea to checksum tables before using them.
     *
     * HINT: Use mmio_map_region/mmio_remap_last_region
     * before accessing table addresses
     * (Why mmio_remap_last_region is required? - Because size of table is
     * unknown prior to mapping it. We need to first map header, than the rest
     * of the table)
     * HINT: RSDP address is stored in uefi_lp->ACPIRoot
     * HINT: You may want to distunguish RSDT/XSDT
     */

    RSDP *rsdp = get_rsdp();

    /*
     * Check parity of RSDP content. ACPI 6.5 specification, section 5.2.5.3
     * says, that only the first 20 bytes can be checked here
     */
    if (!check_parity(rsdp, RSDP_FIRST_BYTES)) {
        panic("Invalid RSDP");
    }

    /*
     * ACPI 1.0
     */
    if (rsdp->Revision == ACPI_VERSION_1_0) {
        RSDT *rsdt = NULL;
        int status = try_get_acpi_table(rsdp->RsdtAddress, (void **)&rsdt);

        if (status < 0) {
            panic("Invalid RSDT: %i", status);
        }

        return acpi_find_table_rsdt(sign, rsdt);
    }
    /*
     * ACPI 2.0 or higher
     */
    if (rsdp->Revision == ACPI_VERSION_2_0_higher) {
        /*
         * Now we can check the whole RSDP
         */
        if (!check_parity(rsdp, sizeof(RSDP))) {
            panic("Invalid Extended RSDP");
        }

        XSDT *xsdt = NULL;
        int status = try_get_acpi_table(rsdp->XsdtAddress, (void **)&xsdt);

        if (status < 0) {
            panic("Invalid XSDT: %i", status);
        }

        return acpi_find_table_xsdt(sign, xsdt);
    }

    panic("Invalid ACPI Version");

    return NULL;
}

MCFG *
get_mcfg(void) {
    static MCFG *kmcfg;
    if (!kmcfg) {
        struct AddressSpace *as = switch_address_space(&kspace);
        kmcfg = acpi_find_table("MCFG");
        switch_address_space(as);
    }

    return kmcfg;
}

#define MAX_SEGMENTS 16

uintptr_t
make_fs_args(char *ustack_top) {

    MCFG *mcfg = get_mcfg();
    if (!mcfg) {
        cprintf("MCFG table is absent!");
        return (uintptr_t)ustack_top;
    }

    char *argv[MAX_SEGMENTS + 3] = {0};

    /* Store argv strings on stack */

    ustack_top -= 3;
    argv[0] = ustack_top;
    nosan_memcpy(argv[0], "fs", 3);

    int nent = (mcfg->h.Length - sizeof(MCFG)) / sizeof(CSBAA);
    if (nent > MAX_SEGMENTS)
        nent = MAX_SEGMENTS;

    for (int i = 0; i < nent; i++) {
        CSBAA *ent = &mcfg->Data[i];

        char arg[64];
        snprintf(arg, sizeof(arg) - 1, "ecam=%llx:%04x:%02x:%02x",
                 (long long)ent->BaseAddress, ent->SegmentGroup, ent->StartBus, ent->EndBus);

        int len = strlen(arg) + 1;
        ustack_top -= len;
        nosan_memcpy(ustack_top, arg, len);
        argv[i + 1] = ustack_top;
    }

    char arg[64];
    snprintf(arg, sizeof(arg) - 1, "tscfreq=%llx", (long long)tsc_calibrate());
    int len = strlen(arg) + 1;
    ustack_top -= len;
    nosan_memcpy(ustack_top, arg, len);
    argv[nent + 1] = ustack_top;

    /* Realign stack */
    ustack_top = (char *)((uintptr_t)ustack_top & ~(2 * sizeof(void *) - 1));

    /* Copy argv vector */
    ustack_top -= (nent + 3) * sizeof(void *);
    nosan_memcpy(ustack_top, argv, (nent + 3) * sizeof(argv[0]));

    char **argv_arg = (char **)ustack_top;
    long argc_arg = nent + 2;

    /* Store argv and argc arguemnts on stack */
    ustack_top -= sizeof(void *);
    nosan_memcpy(ustack_top, &argv_arg, sizeof(argv_arg));
    ustack_top -= sizeof(void *);
    nosan_memcpy(ustack_top, &argc_arg, sizeof(argc_arg));

    /* and return new stack pointer */
    return (uintptr_t)ustack_top;
}

/* Obtain and map FADT ACPI table address. */
FADT *
get_fadt(void) {
    /*
     * ACPI Specification Release 6.5
     * Section 5.2.9
     */
    return acpi_find_table("FACP");
}

/* Obtain and map RSDP ACPI table address. */
HPET *
get_hpet(void) {
    /*
     * IA-PC HPET Specification Revision 1.0a
     * Section 3.2.4
     */
    return acpi_find_table("HPET");
}

/* Getting physical HPET timer address from its table. */
HPETRegister *
hpet_register(void) {
    HPET *hpet_timer = get_hpet();
    if (!hpet_timer->address.address) panic("hpet is unavailable\n");

    uintptr_t paddr = hpet_timer->address.address;
    return mmio_map_region(paddr, sizeof(HPETRegister));
}

/* Debug HPET timer state. */
void
hpet_print_struct(void) {
    HPET *hpet = get_hpet();
    assert(hpet != NULL);
    cprintf("signature = %s\n", (hpet->h).Signature);
    cprintf("length = %08x\n", (hpet->h).Length);
    cprintf("revision = %08x\n", (hpet->h).Revision);
    cprintf("checksum = %08x\n", (hpet->h).Checksum);

    cprintf("oem_revision = %08x\n", (hpet->h).OEMRevision);
    cprintf("creator_id = %08x\n", (hpet->h).CreatorID);
    cprintf("creator_revision = %08x\n", (hpet->h).CreatorRevision);

    cprintf("hardware_rev_id = %08x\n", hpet->hardware_rev_id);
    cprintf("comparator_count = %08x\n", hpet->comparator_count);
    cprintf("counter_size = %08x\n", hpet->counter_size);
    cprintf("reserved = %08x\n", hpet->reserved);
    cprintf("legacy_replacement = %08x\n", hpet->legacy_replacement);
    cprintf("pci_vendor_id = %08x\n", hpet->pci_vendor_id);
    cprintf("hpet_number = %08x\n", hpet->hpet_number);
    cprintf("minimum_tick = %08x\n", hpet->minimum_tick);

    cprintf("address_structure:\n");
    cprintf("address_space_id = %08x\n", (hpet->address).address_space_id);
    cprintf("register_bit_width = %08x\n", (hpet->address).register_bit_width);
    cprintf("register_bit_offset = %08x\n", (hpet->address).register_bit_offset);
    cprintf("address = %08lx\n", (unsigned long)(hpet->address).address);
}

static volatile HPETRegister *hpetReg;
/* HPET timer period (in femtoseconds) */
static uint64_t hpetFemto = 0;
/* HPET timer frequency */
static uint64_t hpetFreq = 0;

/* HPET timer initialisation */
void
hpet_init() {
    if (hpetReg == NULL) {
        nmi_disable();
        hpetReg = hpet_register();
        uint64_t cap = hpetReg->GCAP_ID;
        hpetFemto = (uintptr_t)(cap >> 32);

        if (!(cap & HPET_LEG_RT_CAP)) panic("HPET has no LegacyReplacement mode");

        // cprintf("hpetFemto = %llu\n", hpetFemto);
        hpetFreq = (1 * Peta) / hpetFemto;
        // cprintf("HPET: Frequency = %lu.%03luMHz\n", (uintptr_t)(hpetFreq / Mega), (uintptr_t)(hpetFreq % Mega));

        /* Make interrupts periodic */
        if (!(hpetReg->TIM0_CONF & HPET_TN_PER_INT_CAP)) panic("HPET0 has no periodic interrupts");
        if (!(hpetReg->TIM1_CONF & HPET_TN_PER_INT_CAP)) panic("HPET1 has no periodic interrupts");

        hpetReg->TIM0_CONF |= HPET_TN_TYPE_CNF;
        hpetReg->TIM1_CONF |= HPET_TN_TYPE_CNF;

        const uint64_t half_second = hpetFreq / 2;
        /* Initialize timer frequencies */
        hpetReg->TIM0_CONF |= HPET_TN_VAL_SET_CNF;
        hpetReg->TIM0_COMP = half_second;
        hpetReg->TIM0_COMP = half_second;
        hpetReg->TIM1_CONF |= HPET_TN_VAL_SET_CNF;
        hpetReg->TIM1_COMP = 3 * half_second;
        hpetReg->TIM1_COMP = 3 * half_second;

        /* Start timers */
        hpetReg->TIM0_CONF |= HPET_TN_INT_ENB_CNF;
        hpetReg->TIM1_CONF |= HPET_TN_INT_ENB_CNF;

        hpet_print_reg();

        hpetReg->GEN_CONF |= HPET_LEG_RT_CNF;
        /* Enable ENABLE_CNF bit to enable timer */
        hpetReg->GEN_CONF |= HPET_ENABLE_CNF;
        nmi_enable();
    }
}

/* HPET register contents debugging. */
void
hpet_print_reg(void) {
    cprintf("GCAP_ID = %016lx\n", (unsigned long)hpetReg->GCAP_ID);
    cprintf("GEN_CONF = %016lx\n", (unsigned long)hpetReg->GEN_CONF);
    cprintf("GINTR_STA = %016lx\n", (unsigned long)hpetReg->GINTR_STA);
    cprintf("MAIN_CNT = %016lx\n", (unsigned long)hpetReg->MAIN_CNT);
    cprintf("TIM0_CONF = %016lx\n", (unsigned long)hpetReg->TIM0_CONF);
    cprintf("TIM0_COMP = %016lx\n", (unsigned long)hpetReg->TIM0_COMP);
    cprintf("TIM0_FSB = %016lx\n", (unsigned long)hpetReg->TIM0_FSB);
    cprintf("TIM1_CONF = %016lx\n", (unsigned long)hpetReg->TIM1_CONF);
    cprintf("TIM1_COMP = %016lx\n", (unsigned long)hpetReg->TIM1_COMP);
    cprintf("TIM1_FSB = %016lx\n", (unsigned long)hpetReg->TIM1_FSB);
    cprintf("TIM2_CONF = %016lx\n", (unsigned long)hpetReg->TIM2_CONF);
    cprintf("TIM2_COMP = %016lx\n", (unsigned long)hpetReg->TIM2_COMP);
    cprintf("TIM2_FSB = %016lx\n", (unsigned long)hpetReg->TIM2_FSB);
}

/* HPET main timer counter value. */
uint64_t
hpet_get_main_cnt(void) {
    return hpetReg->MAIN_CNT;
}

/* - Configure HPET timer 0 to trigger every 0.5 seconds on IRQ_TIMER line
 * - Configure HPET timer 1 to trigger every 1.5 seconds on IRQ_CLOCK line
 *
 * HINT To be able to use HPET as PIT replacement consult
 *      LegacyReplacement functionality in HPET spec.
 * HINT Don't forget to unmask interrupt in PIC */
void
hpet_enable_interrupts_tim0(void) {
    if (hpetReg == NULL) {
        panic("Failed to enable timer interrupts: HPET is not initialized!");
    }
    const uint64_t period = hpetFreq / 2;

    nmi_disable();

    /* Stop main counter */
    hpetReg->GEN_CONF &= ~HPET_ENABLE_CNF;

    /* Enable LegacyReplacement mode */
    hpetReg->GEN_CONF |= HPET_LEG_RT_CNF;

    /* Ensure timer can generate periodic interrupts */
    if ((hpetReg->TIM0_CONF & HPET_TN_PER_INT_CAP) == 0) {
        panic("HPET0 has no periodic interrupts");
    }
    /* Make interrupts periodic */
    hpetReg->TIM0_CONF |= HPET_TN_TYPE_CNF;

    /* Set time of first activation */
    hpetReg->TIM0_CONF |= HPET_TN_VAL_SET_CNF;
    hpetReg->TIM0_COMP = hpetReg->MAIN_CNT + period;

    /* Set timer period */
    hpetReg->TIM0_COMP = period;

    /* Start timer */
    hpetReg->TIM0_CONF |= HPET_TN_INT_ENB_CNF;

    /* Enable main counter back */
    hpetReg->GEN_CONF |= HPET_ENABLE_CNF;

    /* Unmask interrupt */
    pic_irq_unmask(IRQ_TIMER);

    nmi_enable();
}

void
hpet_enable_interrupts_tim1(void) {
    if (hpetReg == NULL) {
        panic("Failed to enable clock interrupts: HPET is not initialized!");
    }
    const uint64_t period = (hpetFreq / 2) * 3;

    nmi_disable();

    /* Stop main counter */
    hpetReg->GEN_CONF &= ~HPET_ENABLE_CNF;

    /* Enable LegacyReplacement mode */
    hpetReg->GEN_CONF |= HPET_LEG_RT_CNF;

    /* Ensure timer can generate periodic interrupts */
    if ((hpetReg->TIM1_CONF & HPET_TN_PER_INT_CAP) == 0) {
        panic("HPET1 has no periodic interrupts");
    }
    /* Make interrupts periodic */
    hpetReg->TIM1_CONF |= HPET_TN_TYPE_CNF;

    /* Set time of first activation */
    hpetReg->TIM1_CONF |= HPET_TN_VAL_SET_CNF;
    hpetReg->TIM1_COMP = hpetReg->MAIN_CNT + period;

    /* Set timer period */
    hpetReg->TIM1_COMP = period;

    /* Start timer */
    hpetReg->TIM1_CONF |= HPET_TN_INT_ENB_CNF;

    /* Enable main counter back */
    hpetReg->GEN_CONF |= HPET_ENABLE_CNF;

    /* Unmask interrupt */
    pic_irq_unmask(IRQ_CLOCK);

    nmi_enable();
}

void
hpet_handle_interrupts_tim0(void) {
    pic_send_eoi(IRQ_TIMER);

    sched_yield();
}

void
hpet_handle_interrupts_tim1(void) {
    pic_send_eoi(IRQ_CLOCK);

    sched_yield();
}

/* Calculate CPU frequency in Hz with the help with HPET timer.
 * HINT Use hpet_get_main_cnt function and do not forget about
 * about pause instruction. */
uint64_t
hpet_cpu_frequency(void) {
    static uint64_t cpu_freq;

    if (cpu_freq != 0) {
        return cpu_freq;
    }

    if (hpetReg == NULL) {
        panic("HPET is not initialized");
    }

    /* Spend around 100ms on measurements */
    uint64_t target_delay = hpetFreq / 10;
    uint64_t current_delay = 0;

    /* Spend at least 1 HPET tick on measurements */
    if (target_delay == 0) {
      target_delay = 1;
    }

    uint64_t hpet_start = hpet_get_main_cnt();
    uint64_t tsc_start = read_tsc();
    uint64_t hpet_end = 0;

    do {
        asm volatile("pause");
        hpet_end = hpet_get_main_cnt();

        if (hpet_start <= hpet_end) {
            /* Counter has not rolled over */
            current_delay = hpet_end - hpet_start;
        } else {
            /* 64-bit counter rolled over (very unlikely, but possible) */
            current_delay = 0xFFFFFFFFFFFFFFFF - hpet_start + hpet_end;
        }
    } while (current_delay < target_delay);

    uint64_t tsc_end = read_tsc();
    if (tsc_end < tsc_start) {
        panic("Non-monotonic TSC!");
    }
    cpu_freq = (tsc_end - tsc_start) * hpetFreq / current_delay;

    return cpu_freq;
}

uint32_t
pmtimer_get_timeval(void) {
    FADT *fadt = get_fadt();
    return inl(fadt->PMTimerBlock);
}

/* Calculate CPU frequency in Hz with the help with ACPI PowerManagement timer.
 * HINT Use pmtimer_get_timeval function and do not forget that ACPI PM timer
 *      can be 24-bit or 32-bit. */
uint64_t
pmtimer_cpu_frequency(void) {
    static uint64_t cpu_freq;

    if (cpu_freq != 0) {
        return cpu_freq;
    }

    /* Spend around 100ms on measurements */
    uint64_t target_delay = PM_FREQ / 10;
    uint64_t current_delay = 0;

    uint64_t pm_start = pmtimer_get_timeval();
    uint64_t tsc_start = read_tsc();
    uint64_t pm_end = 0;

    do {
        asm volatile("pause");
        pm_end = pmtimer_get_timeval();

        if (pm_start <= pm_end) {
            /* Counter has not rolled over */
            current_delay = pm_end - pm_start;
        } else if (pm_start - pm_end <= 0x00FFFFFF){
            /* 24-bit counter rolled over */
            current_delay = 0x00FFFFFF - pm_start + pm_end;
        } else {
            /* 32-bit counter rolled over */
            current_delay = 0xFFFFFFFF - pm_start + pm_end;
        }
    } while (current_delay < target_delay);

    uint64_t tsc_end = read_tsc();
    if (tsc_end < tsc_start) {
        panic("Non-monotonic TSC!");
    }
    cpu_freq = (tsc_end - tsc_start) * PM_FREQ / current_delay;

    return cpu_freq;
}
