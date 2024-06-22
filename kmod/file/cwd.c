#include "cwd.h"

#include <inc/fs.h>
#include <inc/assert.h>
#include <inc/string.h>
#include <inc/error.h>

struct CwdDesc {
    envid_t env;
    char path[MAXPATHLEN];
};

static struct CwdDesc WorkingDirs[NENV] = {};

const char*
filed_get_env_cwd(envid_t env) {
    const struct CwdDesc* cwd = WorkingDirs + ENVX(env);
    if (cwd->env == env && *cwd->path)
        return cwd->path;
    return NULL;
}

void
filed_set_env_cwd(envid_t env, const char* cwd) {
    const size_t len = strnlen(cwd, MAXPATHLEN);
    assert(len < MAXPATHLEN);
    assert(*cwd);
    assert(*cwd == '/');

    struct CwdDesc* desc = WorkingDirs + ENVX(env);
    desc->env = env;
    strncpy(desc->path, cwd, MAXPATHLEN);
    printf("setted path = %s\n", desc->path);
}

int
filed_get_absolute_path(envid_t env, const char* path, const char** res) {
    static char abs_path[MAXPATHLEN] = "";
    assert(*path);
    const size_t path_len = strnlen(path, MAXPATHLEN);
    if (path_len >= MAXPATHLEN) {
        *res = NULL;
        return -E_INVAL;
    }

    if (*path == '/') {
        *res = path;
        return 0;
    }

    const char* base = filed_get_env_cwd(env);
    if (!base) {
        *res = NULL;
        return -E_NO_CWD;
    }

    const size_t base_len = strnlen(base, MAXPATHLEN);
    const bool has_separator = (base[base_len - 1] == '/');

    if (base_len + path_len + !has_separator >= MAXPATHLEN) {
        *res = NULL;
        return -E_INVAL;
    }

    strncpy(abs_path, base, base_len);
    if (!has_separator) {
        abs_path[base_len] = '/';
    }
    strncpy(abs_path + base_len + !has_separator, path, path_len + 1);

    *res = abs_path;
    return 0;
}
