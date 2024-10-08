/**
 * @file spawn.h
 * @author MeerkatBoss (solodovnikov.ia@phystech.edu)
 *
 * @brief
 *
 * @version 0.1
 * @date 2024-06-10
 *
 * @copyright Copyright MeerkatBoss (c) 2024
 */
#ifndef __KMOD_SPAWN_SPAWN_H
#define __KMOD_SPAWN_SPAWN_H

#include <inc/env.h>

envid_t initd_fork(envid_t parent);
int initd_spawn(envid_t parent, const char *prog, const char **argv);

#endif /* spawn.h */
