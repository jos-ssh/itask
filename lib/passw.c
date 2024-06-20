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

    for (int i = 0; i < n_of_fields; ++i) {
        res[i] = *line == ':' ? NULL : line;

        line = strchr(line, ':');
        if (line) ++line;
    }
}

int
find_line(const char* username, const char* path_to_file,
          char* buff, const size_t size, const char** result, const size_t n_of_fields) {

    struct Fd* file = fopen(path_to_file, O_RDONLY);
    if (!file)
        return -E_BAD_PATH;

    int res = -E_NO_ENT;

    while (fgets(buff, size, file)) {
        parse_line(result, n_of_fields, buff);

        // result[0] must be username
        if (strncmp(result[0], username, strlen(username))) {
            res = 0;
            break;
        }
    }

    fclose(file);
    return res;
}
