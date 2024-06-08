#include "acpi.h"

#include <inc/acpi.h>
#include <inc/assert.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/uefi.h>
#include <inc/lib.h>

#include "inc/types.h"
#include "mmio.h"

#define ACPI_MAX_TABLES 1024

#define RSDP_FIRST_BYTES        20
#define ACPI_VERSION_1_0        0
#define ACPI_VERSION_2_0_higher 2
#define ACPI_SIGN_LENGTH        4

static ACPISDTHeader *Tables[ACPI_MAX_TABLES];

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
    if (!table_header) {
        return -E_NO_MEM;
    }
    table_header = mmio_remap_last_region(
            phys_addr,
            table_header,
            sizeof(ACPISDTHeader),
            table_header->Length);
    if (!table_header) {
        return -E_NO_MEM;
    }

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
    for (size_t i = 0; i < xsdt_entry_count && i < ACPI_MAX_TABLES; ++i) {
        ACPISDTHeader *table = NULL;
        if (Tables[i]) {
            table = Tables[i];
        } else {
            int status = try_get_acpi_table(xsdt->PointerToOtherSDT[i], (void **)&table);
            if (status < 0) {
                panic("Error getting ACPI table: %i", status);
            }
            Tables[i] = table;
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
    for (size_t i = 0; i < rsdt_entry_count && i < ACPI_MAX_TABLES; ++i) {
        ACPISDTHeader *table = NULL;
        if (Tables[i]) {
            table = Tables[i];
        } else {
            int status = try_get_acpi_table(rsdt->PointerToOtherSDT[i], (void **)&table);
            if (status < 0) {
                panic("Error getting ACPI table: %i", status);
            }
            Tables[i] = table;
        }

        if (strncmp(sign, table->Signature, ACPI_SIGN_LENGTH) == 0) {
            return table;
        }
    }

    return NULL;
}

static RSDP *
get_rsdp(void) {
    static RSDP *rsdp = NULL;
    if (rsdp) {
        return rsdp;
    }
    physaddr_t rsdp_paddr = 0;
    sys_get_rsdp_paddr(&rsdp_paddr);

    rsdp = mmio_map_region(rsdp_paddr, RSDP_FIRST_BYTES);
    assert(rsdp);

    /*
     * Check parity of RSDP content. ACPI 6.5 specification, section 5.2.5.3
     * says, that only the first 20 bytes can be checked here
     */
    assert(check_parity(rsdp, RSDP_FIRST_BYTES));

    if (rsdp->Length > RSDP_FIRST_BYTES) {
        rsdp = mmio_remap_last_region(rsdp_paddr, rsdp, RSDP_FIRST_BYTES, rsdp->Length);
        assert(rsdp);

        /*
         * Now we can check the whole RSDP
         */
        assert(check_parity(rsdp, rsdp->Length));
    }

    return rsdp;
}

static RSDT *rsdt = NULL;
static XSDT *xsdt = NULL;

static void acpi_init() {
    RSDP *rsdp = get_rsdp();
    /*
     * ACPI 1.0
     */
    if (rsdp->Revision == ACPI_VERSION_1_0) {
        int status = try_get_acpi_table(rsdp->RsdtAddress, (void **)&rsdt);
        if (status < 0) {
            panic("Invalid RSDT: %i", status);
        }
        return;
    }
    /*
     * ACPI 2.0 or higher
     */
    if (rsdp->Revision == ACPI_VERSION_2_0_higher) {
        int status = try_get_acpi_table(rsdp->XsdtAddress, (void **)&xsdt);
        if (status < 0) {
            panic("Invalid XSDT: %i", status);
        }
        return;
    }

    panic("Invalid ACPI Version");
}

ACPISDTHeader *
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

    if (!rsdt && !xsdt) { acpi_init(); }

    if (xsdt) {
        return acpi_find_table_xsdt(sign, xsdt);
    }
    assert(rsdt);
    return acpi_find_table_rsdt(sign, rsdt);
}
