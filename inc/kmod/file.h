/**
 * @file file.c
 * @author MeerkatBoss (solodovnikov.ia@phystech.edu)
 *
 * @brief
 *
 * @version 0.1
 * @date 2024-06-17
 *
 * @copyright Copyright MeerkatBoss (c) 2024
 */
#ifndef __INC_KMOD_FILE_C
#define __INC_KMOD_FILE_C

#include <inc/kmod/request.h>
#include <inc/fs.h>

#ifndef FILED_VERSION
#define FILED_VERSION 0
#endif // !FILED_VERSION

#define FILED_MODNAME "jos.core.fs_wrapper"

enum FiledRequestType {
    FILED_REQ_IDENTIFY = KMOD_REQ_IDENTIFY,

    FILED_REQ_OPEN = KMOD_REQ_FIRST_USABLE,
    FILED_REQ_CLOSE,
    FILED_REQ_REMOVE,
    FILED_REQ_STAT,
    FILED_REQ_READ,
    FILED_REQ_WRITE,
    FILED_REQ_TRUNCATE,
    FILED_REQ_FLUSH,
    FILED_REQ_SYNC,

    FILED_NREQUESTS
};

union FiledRequest {
    struct FiledOpen {
        char req_path[MAXPATHLEN];
        int req_omode;
        int req_flags;
    } open;
    struct FiledClose {
        int req_fileid;
    } close;
    struct FiledRemove {
        char req_path[MAXPATHLEN];
    } remove;
    struct FiledStat {
        int req_fileid;
    } stat;
    struct FiledRead {
        int req_fileid;
        size_t req_n;
    } read;
    struct FiledWrite {
        int req_fileid;
        size_t req_n;
        char req_buf[PAGE_SIZE - (2 * sizeof(size_t))];
    } write;
    struct FiledTruncate {
        int req_fileid;
        off_t req_size;
    } truncate;
    struct FiledFlush {
        int req_fileid;
    } flush;

    uint8_t pad_[PAGE_SIZE];
} __attribute((aligned(PAGE_SIZE)));

union FiledResponse {
    struct FiledStatRes {
      uint32_t st_mode;
      uint64_t st_uid;
      uint64_t st_gid;
      uint64_t st_size;
    } stat;
    char read_buf[PAGE_SIZE];

    uint8_t pad_[PAGE_SIZE];
} __attribute__((aligned(PAGE_SIZE)));


#endif /* file.c */
