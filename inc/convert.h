/**
 * @file convert.h
 * @author MeerkatBoss (solodovnikov.ia@phystech.edu)
 *
 * @brief
 *
 * @version 0.1
 * @date 2024-05-19
 *
 * @copyright Copyright MeerkatBoss (c) 2024
 */
#ifndef __INC_CONVERT_H
#define __INC_CONVERT_H

#include <inc/types.h>

#define MAX_BASE 64
#define MIN_BASE 2

#define BASE_HEX 16
#define BASE_OCT 8
#define BASE_BIN 2
#define BASE_DEC 10

int str_to_ulong(const char* str, uint8_t base, unsigned long* out);
int str_to_long(const char* str, uint8_t base, long* out);
int strn_to_ulong(const char* str, size_t n, uint8_t base, unsigned long* out);
int strn_to_long(const char* str, size_t n, uint8_t base, long* out);

int ulong_to_str(unsigned long val, uint8_t base, char buffer[], size_t n);
int long_to_str(long val, uint8_t base, char buffer[], size_t n);

#endif /* convert.h */
