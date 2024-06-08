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
#ifndef __INC_ACPI_H
#define __INC_ACPI_H

#include <inc/types.h>

#pragma pack(push, 1)

typedef struct {
    char Signature[8];
    uint8_t Checksum;
    char OEMID[6];
    uint8_t Revision;
    uint32_t RsdtAddress;
    uint32_t Length;
    uint64_t XsdtAddress;
    uint8_t ExtendedChecksum;
    uint8_t reserved[3];
} RSDP;

typedef struct {
    char Signature[4];
    uint32_t Length;
    uint8_t Revision;
    uint8_t Checksum;
    char OEMID[6];
    char OEMTableID[8];
    uint32_t OEMRevision;
    uint32_t CreatorID;
    uint32_t CreatorRevision;
} ACPISDTHeader;

typedef struct {
    ACPISDTHeader h;
    uint32_t PointerToOtherSDT[];
} RSDT;

typedef struct {
    ACPISDTHeader h;
    uint64_t PointerToOtherSDT[];
} XSDT;

#pragma pack(pop)

#endif /* acpi.h */
