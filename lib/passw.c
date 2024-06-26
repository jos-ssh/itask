#include "inc/convert.h"
#include <inc/kmod/users.h>
#include <inc/passw.h>

#include <inc/string.h>
#include <inc/assert.h>
#include <inc/stdio.h>
#include <inc/error.h>

#include <inc/lib.h> // for O_RDONLY

void
parse_line(const char** res, size_t n_of_fields, const char* line) {
    assert(res);
    assert(line);

    // cprintf("%s:%d %lu\n\n", __func__, __LINE__, n_of_fields);
    for (int i = 0; i < n_of_fields; ++i) {
        res[i] = *line == ':' ? NULL : line;

        // cprintf("%s: line '%s'\n", __func__, line);
        if (*line != '\n' && *line != '\0')
            line = strchr(line, ':');
        // cprintf("%s:%d\n", __func__, __LINE__);
        if (line) ++line;
    }
}

int
find_line(const char* username, const char* path_to_file,
          char* buff, const size_t size, const char** result, const size_t n_of_fields) {

    int fd = open_raw_fs(path_to_file, O_RDONLY);
    struct Fd* file;
    int res = fd_lookup(fd, &file);
    if (res)
        return res;

    res = -E_NO_ENT;

    while (fgets(buff, size, file)) {
        if (*buff != '\n')
            parse_line(result, n_of_fields, buff);

        // result[0] must be username
        if (!strncmp(result[0], username, result[1] - result[0] - 1)) {
            res = 0;
            break;
        }
    }

    close(fd);
    return res;
}


int
find_line_user(const char* username, const char* path_to_file,
               char* buff, const size_t size, const char** result, const size_t n_of_fields) {

    int fd = open(path_to_file, O_RDONLY);
    if (fd < 0) {
        cprintf("res: %i\n", fd);
    }
    struct Fd* file;
    int res = fd_lookup(fd, &file);
    if (res)
        return res;

    res = -E_NO_ENT;

    while (fgets(buff, size, file)) {
        if (*buff != '\n')
            parse_line(result, n_of_fields, buff);

        // result[0] must be username
        if (!strncmp(result[0], username, result[1] - result[0] - 1)) {
            res = 0;
            break;
        }
    }

    close(fd);
    return res;
}

int
get_username(uid_t uid, char* out_buffer) {
    int fd = open(PASSWD_PATH, O_RDONLY);
    if (fd < 0) {
        cprintf("res: %i\n", fd);
    }
    struct Fd* file;
    int res = fd_lookup(fd, &file);
    if (res)
        return res;

    res = -E_NO_ENT;

    struct PasswParsed result;
    char buff[MAX_PASSWORD_LENGTH];

    while (fgets(buff, MAX_PASSWORD_LENGTH, file)) {
        if (*buff != '\n') {
            parse_line((const char**)&result, sizeof(result) / sizeof(char*), buff);
        }

        // result[0] must be username
        long found_uid = 0;
        strn_to_long(result.uid, strchr(result.uid, ':') - result.uid, 10, &found_uid);

        if (uid == found_uid) {
            strncpy(out_buffer, result.username, strchr(result.username, ':') - result.username);
            res = 0;
            break;
        }
    }

    close(fd);
    return res;
}
