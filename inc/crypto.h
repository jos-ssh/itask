#ifndef JOS_INC_CRYPTO_H
#define JOS_INC_CRYPTO_H

#include <inc/types.h>

#define KEY_LENGTH 13
#define SALT_LENGTH 7

bool check_PBKDF2(const char* hashed, const char* salt, const char* password);
void get_PBKDF2(const char *password, const char *salt, unsigned char *key_buf);
#endif