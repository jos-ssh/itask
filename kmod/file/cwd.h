/**
 * @file cwd.h
 * @author MeerkatBoss (solodovnikov.ia@phystech.edu)
 *
 * @brief
 *
 * @version 0.1
 * @date 2024-06-18
 *
 * @copyright Copyright MeerkatBoss (c) 2024
 */
#ifndef __KMOD_FILE_CWD_H
#define __KMOD_FILE_CWD_H

#include <inc/env.h>

const char* filed_get_env_cwd(envid_t env);
void filed_set_env_cwd(envid_t env, const char* cwd);

int filed_get_absolute_path(envid_t env, const char* path, const char** abs_path);

#endif /* cwd.h */
