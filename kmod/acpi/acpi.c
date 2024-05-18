#include "acpi.h"
#include "inc/acpi.h"

#include <inc/assert.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/uefi.h>
#include <inc/lib.h>

#define RSDP_FIRST_BYTES        20
#define ACPI_VERSION_1_0        0
#define ACPI_VERSION_2_0_higher 2
#define ACPI_SIGN_LENGTH        4

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
    /*
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
    */
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

static RSDP *
get_rsdp(void) {
    return NULL;
    // return mmio_map_region(uefi_lp->ACPIRoot, sizeof(RSDP));
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
