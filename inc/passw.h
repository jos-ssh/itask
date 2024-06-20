#pragma once

#include <inc/types.h>

#define PASSWD_PATH "/passwd"
#define SHADOW_PATH "/shadow"


/* Parsed line of etc/passwd
   username:UID:GID:homedir:shell

   P.s. Must be implemented as array of char*
*/
struct PasswParsed {
    const char* username;
    const char* uid;
    const char* gid;
    const char* homedir;
    const char* shell;
};

/* Parsed line of etc/passwrd
   username:salt:hashed:last_change_date
*/
struct ShadowParsed {
    const char* username;
    const char* salt;
    const char* hashed;
    const char* last_change_date;
};

/**
 * @brief Parse the line that has 'n_of_fields' entries separated by ':'
 * 
 * @param res is an array of char*
 * @param n_of_fields 
 * @param line 
 */
void parse_line(const char** res, size_t n_of_fields, const char* line);

inline void
parse_passw_line(struct PasswParsed* p, const char* line) {
    return parse_line((const char**) p, sizeof(*p) / sizeof(char*), line);
}

inline void
parse_shadow_line(struct ShadowParsed* s, const char* line) {
   return parse_line((const char**) s, sizeof(*s) / sizeof(char*), line);
}


/**
 * @brief Find user in in 'path_to_file' file with PasswParsed structure
 *        copy thi line to 'buff' and parse it in 'result'
 *
 * P.s 'result' must be an array of char* with 'n_of_fields' entries
 * @return 0 on success
 */
int find_line(const char* username, const char* path_to_file,
              char* buff, size_t size, const char** result, const size_t n_of_fields);


inline int
find_passw_line(const char* username, char* buff, size_t size,
                 struct PasswParsed* result) {
    return find_line(username, PASSWD_PATH, buff, size, (const char**) result, sizeof(*result) / sizeof(char*));
}

inline int
find_shadow_line(const char* username, char* buff, size_t size,
                 struct ShadowParsed* result) {
    return find_line(username, SHADOW_PATH, buff, size, (const char**) result, sizeof(*result) / sizeof(char*));
}