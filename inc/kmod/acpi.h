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
#ifndef __INC_KMOD_ACPI_H
#define __INC_KMOD_ACPI_H

#include "inc/acpi.h"
#include <inc/kmod/request.h>
#include <stdint.h>

#ifndef ACPID_VERSION
#define ACPID_VERSION 0
#endif // !ACPID_VERSION

#define ACPID_MODNAME "jos.core.acpi"

enum AcpidRequestType {
  ACPID_REQ_IDENTIFY = KMOD_REQ_IDENTIFY,

  ACPID_REQ_FIND_TABLE = KMOD_REQ_FIRST_USABLE,

  ACPID_NREQUESTS
};

union AcpidRequest {
  struct AcpidFindTable {
    char Signature[4];
    size_t Offset;
  } find_table;

  uint8_t pad_[PAGE_SIZE];
} __attribute__((aligned(PAGE_SIZE)));

union AcpidResponse {
  struct AcpidTableStart {
    ACPISDTHeader Header;
    uint8_t Data[PAGE_SIZE - sizeof(ACPISDTHeader)];
  } table_start;
  struct AcpiTableSlice {
    uint8_t Data[PAGE_SIZE];
  } table_slice;

  uint8_t pad_[PAGE_SIZE];
} __attribute__((aligned(PAGE_SIZE)));

#endif /* acpi.h */
