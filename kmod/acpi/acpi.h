/**
 * @file acpi.h
 * @author MeerkatBoss (solodovnikov.ia@phystech.edu)
 *
 * @brief
 *
 * @version 0.1
 * @date 2024-05-18
 *
 * @copyright Copyright MeerkatBoss (c) 2024
 */
#ifndef __KMOD_ACPI_ACPI_H
#define __KMOD_ACPI_ACPI_H

#include <inc/acpi.h>

ACPISDTHeader* acpi_find_table(const char* sign);

#endif /* acpi.h */
