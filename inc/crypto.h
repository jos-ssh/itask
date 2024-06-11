#ifndef JOS_INC_CRYPTO_H
#define JOS_INC_CRYPTO_H

#include <inc/types.h>

#define KEY_LENGTH 32

bool check_PBKDF2(const char* hash, const char* salt, const char* password);

#endif