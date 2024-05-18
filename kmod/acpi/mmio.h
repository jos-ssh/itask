/**
 * @file mmio.h
 * @author MeerkatBoss (solodovnikov.ia@phystech.edu)
 *
 * @brief
 *
 * @version 0.1
 * @date 2024-05-19
 *
 * @copyright Copyright MeerkatBoss (c) 2024
 */
#ifndef __KMOD_ACPI_MMIO_H
#define __KMOD_ACPI_MMIO_H

#include "inc/types.h"

void* mmio_map_region(physaddr_t paddr, size_t size);
void* mmio_remap_last_region(physaddr_t paddr, void* old_vaddr, size_t old_size, size_t new_size);

#endif /* mmio.h */
