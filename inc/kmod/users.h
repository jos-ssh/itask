#pragma once

#include <inc/kmod/request.h>
#include <inc/env.h>

#define USERSD_VERSION 0
#define USERSD_MODNAME "jos.core.users"

enum UsersdRequestType {
    USERSD_REQ_IDENTIFY = KMOD_REQ_IDENTIFY,

    USERSD_REQ_LOGIN = KMOD_REQ_FIRST_USABLE,
    USERSD_REQ_REG_ENV,
    USERSD_REQ_GET_ENV_INFO,
    USERSD_REQ_SET_ENV_INFO,

    USERSD_NREQUESTS
};

#define MAX_USERNAME_LENGTH 256
#define MAX_PASSWORD_LENGTH 256
#define MAX_PATH_LEN        256

#define ROOT_UID 0
#define ROOT_GID 0

#define UINT_MAX ((unsigned)(-1))
#define NOT_AN_ID UINT_MAX


struct EnvInfo {
    uid_t ruid;
    uid_t euid;

    gid_t rgid;
    gid_t egid;
};

union UsersdRequest {
    struct UsersdLogin {
        char username[MAX_USERNAME_LENGTH];
        char password[MAX_PASSWORD_LENGTH];
    } login;

    struct UsersdUseradd {
        char username[MAX_USERNAME_LENGTH];
        char passwd[MAX_PASSWORD_LENGTH];
        char homedir[MAX_PATH_LEN];
    } useradd;

    struct UsersdRegisterEnv {
        envid_t parent_pid;
        envid_t child_pid;

        struct EnvInfo desired_child_info;
    } register_env;

    struct UsersdGetEnvInfo {
        envid_t target;
    } get_env_info;

    struct UsersdSetEnvInfo {
        struct EnvInfo info;
    } set_env_info;

    uint8_t pad_[PAGE_SIZE];
} __attribute__((aligned(PAGE_SIZE)));

union UsersdResponse {
    struct UsersdEnvInfo {
        struct EnvInfo info;
    } env_info;

    uint8_t pad_[PAGE_SIZE];
} __attribute__((aligned(PAGE_SIZE)));
