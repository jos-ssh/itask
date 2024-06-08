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

#include <inc/assert.h>
#include <inc/kmod/request.h>
#include <inc/types.h>

#ifndef PCID_VERSION
#define PCID_VERSION 0
#endif // ! PCID_VERSION

#define PCID_MODNAME "jos.driver.pci"

enum PcidRequestType {
    PCID_REQ_IDENTIFY = KMOD_REQ_IDENTIFY,

    PCID_REQ_FIND_DEVICE = KMOD_REQ_FIRST_USABLE,
    PCID_REQ_READ_BAR,
    PCID_REQ_LSPCI,

    PCID_NREQUESTS
};

#define PCID_CLASS_MASK    (0xFF << 24)
#define PCID_SUBCLASS_MASK (0xFF << 16)
#define PCID_PROG_MASK     (0xFF << 8)
#define PCID_BAR_MASK      (0xE0) // 0x07 << 5
#define PCID_REQ_MASK      (0x1F)

/**
 * @brief Pack device identification info. Last byte can be used for request id
 */
__attribute__((always_inline)) inline uint32_t
pcid_device_id(uint8_t cls, uint8_t scls, uint8_t prog) {
    static_assert(PCID_NREQUESTS <= PCID_REQ_MASK, "pcid request mask is invalid");
    return ((uint32_t)cls << 24) | ((uint32_t)scls << 16) | ((uint32_t)prog << 8);
}

/**
 * @brief Pack device BAR identification info.
 * Last byte can be used for request id
 */
__attribute__((always_inline)) inline uint32_t
pcid_bar_id(uint8_t cls, uint8_t scls, uint8_t prog, uint8_t bar) {
    assert(bar < 8);
    return pcid_device_id(cls, scls, prog) | ((uint32_t) bar << 5);
}

union PcidResponse {
    struct PciBar {
      physaddr_t address;
      size_t size;
    } bar;
    physaddr_t dev_confspace;

    uint8_t pad_[PAGE_SIZE];
} __attribute__((aligned(PAGE_SIZE)));


#endif /* pci.h */
