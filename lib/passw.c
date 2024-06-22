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
    assert(res == 0);
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
