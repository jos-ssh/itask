/**
 * @file pci.h
 * @author MeerkatBoss (solodovnikov.ia@phystech.edu)
 *
 * @brief
 *
 * @version 0.1
 * @date 2024-05-29
 *
 * @copyright Copyright MeerkatBoss (c) 2024
 */
#ifndef __INC_KMOD_PCI_H
#define __INC_KMOD_PCI_H

#include <inc/kmod/request.h>
#include <inc/pci.h>
#include <inc/types.h>

#ifndef PCID_VERSION
#define PCID_VERSION 0
#endif // ! PCID_VERSION

#define PCID_MODNAME "jos.driver.pci"

enum PcidRequestType {
    PCID_REQ_IDENTIFY = KMOD_REQ_IDENTIFY,

    PCID_REQ_FIND_DEVICE = KMOD_REQ_FIRST_USABLE,
    PCID_REQ_MAP_REGION,
    PCID_REQ_LSPCI,

    PCID_NREQUESTS
};

#define PCI_CLASS_MASK    (0xFF << 24)
#define PCI_SUBCLASS_MASK (0xFF << 16)
#define PCI_PROG_MASK     (0xFF << 8)
#define PCI_REQ_MASK      (0xFF)

/**
 * @brief Pack device identification info. Last byte can be used for request id
 */
__attribute__((always_inline)) inline uint32_t
pcid_device_id(uint8_t cls, uint8_t scls, uint8_t prog) {
    static_assert(PCID_NREQUESTS <= PCI_REQ_MASK, "pcid request mask is invalid");
    return ((uint32_t)cls << 24) | ((uint32_t)scls << 16) | ((uint32_t)prog << 8);
}

union PcidRequest {
    struct PcidMapRegion {
        void* TargetAddress;
        uint8_t PciBarId;
    } map_region;

    uint8_t pad_[PAGE_SIZE];
} __attribute__((aligned(PAGE_SIZE)));

union PcidResponse {
    struct PciHeader device;

    uint8_t pad_[PAGE_SIZE];
} __attribute__((aligned(PAGE_SIZE)));


#endif /* pci.h */
