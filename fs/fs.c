#include <inc/string.h>
#include <inc/partition.h>

#include "fs.h"
#include "inc/error.h"
#include "inc/fs.h"
#include "inc/lib.h"
#include "inc/stdio.h"
#include "inc/filemode.h"

/* umask */
uint32_t umask = IUMASK;

/* Superblock */
struct Super *super;
/* Bitmap blocks mapped in memory */
uint32_t *bitmap;

/****************************************************************
 *                         Super block
 ****************************************************************/

/* Validate the file system super-block. */
void
check_super(void) {
    if (super->s_magic != FS_MAGIC)
        panic("bad file system magic number %08x", super->s_magic);

    cprintf("file system has 0x%0x blocks\n", super->s_nblocks);
    if (super->s_nblocks > DISKSIZE / BLKSIZE)
        panic("file system is too large");

    cprintf("superblock is good\n");
}

/****************************************************************
 *                         Free block bitmap
 ****************************************************************/

/* Check to see if the block bitmap indicates that block 'blockno' is free.
 * Return 1 if the block is free, 0 if not. */
bool
block_is_free(blockno_t blockno) {
    if (super == 0 || blockno >= super->s_nblocks) return 0;
    if (TSTBIT(bitmap, blockno)) return 1;
    return 0;
}

/* Mark a block free in the bitmap */
void
free_block(blockno_t blockno) {
    /* Blockno zero is the null pointer of block numbers. */
    if (blockno == 0) panic("attempt to free zero block");
    SETBIT(bitmap, blockno);
    //     flush_block(bitmap);
}

/* Search the bitmap for a free block and allocate it.  When you
 * allocate a block, immediately flush the changed bitmap block
 * to disk.
 *
 * Return block number allocated on success,
 * 0 if we are out of blocks.
 *
 * Hint: use free_block as an example for manipulating the bitmap. */
blockno_t
alloc_block(void) {
    /* The bitmap consists of one or more blocks.  A single bitmap block
     * contains the in-use bits for BLKBITSIZE blocks.  There are
     * super->s_nblocks blocks in the disk altogether. */

    for (blockno_t i = 1; i <= super->s_nblocks; i++) {
        if (block_is_free(i)) {
            CLRBIT(bitmap, i);
            assert(is_page_dirty(bitmap + i / 32));
            flush_block(bitmap + i / 32);
            return i;
        }
    }

    return 0;
}

/* Validate the file system bitmap.
 *
 * Check that all reserved blocks -- 0, 1, and the bitmap blocks themselves --
 * are all marked as in-use. */
void
check_bitmap(void) {

    /* Make sure all bitmap blocks are marked in-use */
    for (blockno_t i = 0; i * BLKBITSIZE < super->s_nblocks; i++)
        assert(!block_is_free(2 + i));

    /* Make sure the reserved and root blocks are marked in-use. */

    assert(!block_is_free(1));
    assert(!block_is_free(0));

    cprintf("bitmap is good\n");
}

/****************************************************************
 *                    File system structures
 ****************************************************************/

/* Initialize the file system */
void
fs_init(void) {
    static_assert(sizeof(struct File) == 256, "Unsupported file size");

    bc_init();

    /* Set "super" to point to the super block. */
    super = diskaddr(1);
    check_super();

    /* Set "bitmap" to the beginning of the first bitmap block. */
    bitmap = diskaddr(2);

    check_bitmap();
}

/* Find the disk block number slot for the 'filebno'th block in file 'f'.
 * Set '*ppdiskbno' to point to that slot.
 * The slot will be one of the f->f_direct[] entries,
 * or an entry in the indirect block.
 * When 'alloc' is set, this function will allocate an indirect block
 * if necessary.
 *
 * Returns:
 *  0 on success (but note that *ppdiskbno might equal 0).
 *  -E_NOT_FOUND if the function needed to allocate an indirect block, but
 *      alloc was 0.
 *  -E_NO_DISK if there's no space on the disk for an indirect block.
 *  -E_INVAL if filebno is out of range (it's >= NDIRECT + NINDIRECT).
 *
 * Analogy: This is like pgdir_walk for files.
 * Hint: Don't forget to clear any block you allocate. */
int
file_block_walk(struct File *f, blockno_t filebno, blockno_t **ppdiskbno, bool alloc) {
    if (filebno >= NDIRECT + NINDIRECT || !ppdiskbno) {
        return -E_INVAL;
    }

    *ppdiskbno = NULL;
    if (filebno < NDIRECT) {
        *ppdiskbno = f->f_direct + filebno;
        return 0;
    }
    if (f->f_indirect == 0) {
        if (!alloc) {
            return -E_NOT_FOUND;
        }

        f->f_indirect = alloc_block();
        if (f->f_indirect == 0) {
            return -E_NO_DISK;
        }
        memset(diskaddr(f->f_indirect), 0, BLKSIZE);
    }

    blockno_t *indirect = diskaddr(f->f_indirect);
    *ppdiskbno = &indirect[filebno - NDIRECT];
    return 0;
}

/* Set *blk to the address in memory where the filebno'th
 * block of file 'f' would be mapped.
 *
 * Returns 0 on success, < 0 on error.  Errors are:
 *  -E_NO_DISK if a block needed to be allocated but the disk is full.
 *  -E_INVAL if filebno is out of range.
 *
 * Hint: Use file_block_walk and alloc_block. */
int
file_get_block(struct File *f, blockno_t filebno, char **blk) {
    if (!blk) {
        return -E_INVAL;
    }
    *blk = NULL;

    blockno_t *pblockno = NULL;
    int lookup_res = file_block_walk(f, filebno, &pblockno, true);
    if (lookup_res != 0) {
        return lookup_res;
    }

    if (*pblockno == 0) {
        *pblockno = alloc_block();
        if (*pblockno == 0) {
            return -E_NO_DISK;
        }
    }

    *blk = diskaddr(*pblockno);
    return 0;
}

/* Try to find a file named "name" in dir.  If so, set *file to it.
 *
 * Returns 0 and sets *file on success, < 0 on error.  Errors are:
 *  -E_NOT_FOUND if the file is not found */
static int
dir_lookup(struct File *dir, const char *name, struct File **file) {
    /* Search dir for name.
     * We maintain the invariant that the size of a directory-file
     * is always a multiple of the file system's block size. */
    assert((dir->f_size % BLKSIZE) == 0);
    blockno_t nblock = dir->f_size / BLKSIZE;
    for (blockno_t i = 0; i < nblock; i++) {
        char *blk;
        int res = file_get_block(dir, i, &blk);
        if (res < 0) return res;

        struct File *f = (struct File *)blk;
        for (blockno_t j = 0; j < BLKFILES; j++)
            if (strcmp(f[j].f_name, name) == 0) {
                *file = &f[j];
                return 0;
            }
    }
    return -E_NOT_FOUND;
}

/* Set *file to point at a free File structure in dir.  The caller is
 * responsible for filling in the File fields. */
static int
dir_alloc_file(struct File *dir, struct File **file) {
    char *blk;

    assert((dir->f_size % BLKSIZE) == 0);
    blockno_t nblock = dir->f_size / BLKSIZE;
    for (blockno_t i = 0; i < nblock; i++) {
        int res = file_get_block(dir, i, &blk);
        if (res < 0) return res;

        struct File *f = (struct File *)blk;
        for (blockno_t j = 0; j < BLKFILES; j++) {
            if (f[j].f_name[0] == '\0') {
                *file = &f[j];
                return 0;
            }
        }
    }
    dir->f_size += BLKSIZE;
    int res = file_get_block(dir, nblock, &blk);
    if (res < 0) return res;

    *file = (struct File *)blk;
    return 0;
}

/* Skip over slashes. */
static const char *
skip_slash(const char *p) {
    while (*p == '/')
        p++;
    return p;
}

/* Evaluate a path name, starting at the root.
 * On success, set *pf to the file we found
 * and set *pdir to the directory the file is in.
 * If we cannot find the file but find the directory
 * it should be in, set *pdir and copy the final path
 * element into lastelem. */
static int
walk_path(const char *path, struct File **pdir, struct File **pf, char *lastelem) {
    const char *p;
    char name[MAXNAMELEN];
    struct File *dir, *f;
    int r;

    // if (*path != '/')
    //     return -E_BAD_PATH;
    path = skip_slash(path);
    f = &super->s_root;
    dir = 0;
    name[0] = 0;

    if (pdir)
        *pdir = 0;
    *pf = 0;
    while (*path != '\0') {
        dir = f;
        p = path;
        while (*path != '/' && *path != '\0')
            path++;
        if (path - p >= MAXNAMELEN)
            return -E_BAD_PATH;
        memmove(name, p, path - p);
        name[path - p] = '\0';
        path = skip_slash(path);

        if (!ISDIR(dir->f_mode))
            return -E_NOT_FOUND;

        if ((r = dir_lookup(dir, name, &f)) < 0) {
            if (r == -E_NOT_FOUND && *path == '\0') {
                if (pdir)
                    *pdir = dir;
                if (lastelem)
                    strcpy(lastelem, name);
                *pf = 0;
            }
            return r;
        }
    }

    if (pdir)
        *pdir = dir;
    *pf = f;
    return 0;
}

/****************************************************************
 *                        File operations
 ****************************************************************/

/* Create "path".  On success set *pf to point at the file and return 0.
 * On error return < 0. */
int
file_create(const char *path, struct File **pf, uint32_t mode, uint32_t gid, uint32_t uid) {
    char name[MAXNAMELEN];
    int res;
    struct File *dir, *filp;

    if (!(res = walk_path(path, &dir, &filp, name))) return -E_FILE_EXISTS;
    if (res != -E_NOT_FOUND || dir == 0) return res;
    if ((res = dir_alloc_file(dir, &filp)) < 0) return res;

    strcpy(filp->f_name, name);
    filp->f_gid = gid;
    filp->f_uid = uid;
    filp->f_mode = (mode & ~umask);
    *pf = filp;
    file_flush(dir);
    return 0;
}

/* Open "path".  On success set *pf to point at the file and return 0.
 * On error return < 0. */
int
file_open(const char *path, struct File **pf) {
    int res = walk_path(path, 0, pf, 0);
    return res;
}

/* Read count bytes from f into buf, starting from seek position
 * offset.  This meant to mimic the standard pread function.
 * Returns the number of bytes read, < 0 on error. */
ssize_t
file_read(struct File *f, void *buf, size_t count, off_t offset) {
    char *blk;

    if (offset >= f->f_size)
        return 0;

    count = MIN(count, f->f_size - offset);

    for (off_t pos = offset; pos < offset + count;) {
        int r = file_get_block(f, pos / BLKSIZE, &blk);
        if (r < 0) return r;

        int bn = MIN(BLKSIZE - pos % BLKSIZE, offset + count - pos);
        memmove(buf, blk + pos % BLKSIZE, bn);
        pos += bn;
        buf += bn;
    }

    return count;
}

/* Write count bytes from buf into f, starting at seek position
 * offset.  This is meant to mimic the standard pwrite function.
 * Extends the file if necessary.
 * Returns the number of bytes written, < 0 on error. */
ssize_t
file_write(struct File *f, const void *buf, size_t count, off_t offset) {
    int res;

    /* Extend file if necessary */
    if (offset + count > f->f_size)
        if ((res = file_set_size(f, offset + count)) < 0) return res;

    for (off_t pos = offset; pos < offset + count;) {
        char *blk;
        if ((res = file_get_block(f, pos / BLKSIZE, &blk)) < 0) return res;

        blockno_t bn = MIN(BLKSIZE - pos % BLKSIZE, offset + count - pos);
        memmove(blk + pos % BLKSIZE, buf, bn);
        pos += bn;
        buf += bn;
    }

    return count;
}

/* Remove a block from file f.  If it's not there, just silently succeed.
 * Returns 0 on success, < 0 on error. */
static int
file_free_block(struct File *f, blockno_t filebno) {
    blockno_t *ptr;
    int res = file_block_walk(f, filebno, &ptr, 0);
    if (res < 0) return res;

    if (*ptr) {
        free_block(*ptr);
        *ptr = 0;
    }
    return 0;
}

/* Remove any blocks currently used by file 'f',
 * but not necessary for a file of size 'newsize'.
 * For both the old and new sizes, figure out the number of blocks required,
 * and then clear the blocks from new_nblocks to old_nblocks.
 * If the new_nblocks is no more than NDIRECT, and the indirect block has
 * been allocated (f->f_indirect != 0), then free the indirect block too.
 * (Remember to clear the f->f_indirect pointer so you'll know
 * whether it's valid!)
 * Do not change f->f_size. */
static void
file_truncate_blocks(struct File *f, off_t newsize) {
    blockno_t old_nblocks = CEILDIV(f->f_size, BLKSIZE);
    blockno_t new_nblocks = CEILDIV(newsize, BLKSIZE);
    for (blockno_t bno = new_nblocks; bno < old_nblocks; bno++) {
        int res = file_free_block(f, bno);
        if (res < 0) cprintf("warning: file_free_block: %i", res);
    }

    if (new_nblocks <= NDIRECT && f->f_indirect) {
        free_block(f->f_indirect);
        f->f_indirect = 0;
    }
}

/* Set the size of file f, truncating or extending as necessary. */
int
file_set_size(struct File *f, off_t newsize) {
    if (f->f_size > newsize)
        file_truncate_blocks(f, newsize);
    f->f_size = newsize;

    flush_block(f);
    if (f->f_indirect) {
        flush_block(diskaddr(f->f_indirect));
    }

    flush_bitmap();
    return 0;
}

/* Flush the contents and metadata of file f out to disk.
 * Loop over all the blocks in file.
 * Translate the file block number into a disk block number
 * and then check whether that disk block is dirty.  If so, write it out. */
void
file_flush(struct File *f) {
    blockno_t *pdiskbno;

    for (blockno_t i = 0; i < CEILDIV(f->f_size, BLKSIZE); i++) {
        if (file_block_walk(f, i, &pdiskbno, 0) < 0 ||
            pdiskbno == NULL || *pdiskbno == 0) {
            continue;
        }
        flush_block(diskaddr(*pdiskbno));
    }
    if (f->f_indirect)
        flush_block(diskaddr(f->f_indirect));
    flush_block(f);
    flush_bitmap();
}

/* Sync the entire file system.  A big hammer. */
void
fs_sync(void) {
    for (int i = 1; i < super->s_nblocks; i++) {
        flush_block(diskaddr(i));
    }
}

void
cpy_file_to_file_info(struct FileInfo *file_info, struct File *file) {
    strlcpy(file_info->f_name, file->f_name, MAXNAMELEN);
    file_info->f_size = file->f_size;
    file_info->f_type = file->f_mode;
}

/* Copy to buffer count files, counting start from from_which_count */
int
file_getdents(const char *path, struct FileInfo *buffer, uint32_t count, uint32_t from_which_count) {
    if (debug) {
        printf("Start file_getdents(path = %ld)\n", (uint64_t)path);
        printf("path = <%s>\n", path);
    }

    if (count == 0 || count > MAX_GETDENTS_COUNT)
        return -E_INVAL;
    int res = 0;
    struct File *file = NULL;
    struct File *dir = NULL;
    char lastelem[MAXNAMELEN] = {};

    res = walk_path(path, &dir, &file, lastelem);
    if (res < 0)
        return res;

    assert((file->f_size % BLKSIZE) == 0);
    blockno_t nblock = file->f_size / BLKSIZE;
    int file_counter = 0;
    for (blockno_t i = 0; i < nblock; i++) {

        char *blk;
        res = file_get_block(file, i, &blk);
        if (res < 0) return res;

        struct File *f = (struct File *)blk;
        for (blockno_t j = 0; j < BLKFILES; j++) {
            if (from_which_count > 0) {
                if (debug)
                    printf("from_which_count = %d\n", from_which_count);
                from_which_count--;
                continue;
            }

            if (debug)
                printf("file_counter = %d\n", file_counter);
            cpy_file_to_file_info(&buffer[file_counter], &f[j]);
            file_counter++;
            if (count == file_counter)
                return 0;
        }
    }

    if (debug)
        printf("End file_getdents successfully\n");
    return 0;
}

int
free_file_blocks(struct File *file) {
    int res = 0;

    const blockno_t block_number = file->f_size / BLKSIZE;
    for (blockno_t block_i = 0; block_i < block_number; block_i++) {
        res = file_free_block(file, block_i);
        if (res < 0) return res;
    }

    return 0;
}

/* Remove file
 * return -E_BAD_PATH if path to directory
 */
int
file_remove(const char *path) {
    struct File *dir = NULL;
    struct File *file = NULL;
    char lastelem[MAXPATHLEN] = {};

    int res = walk_path(path, &dir, &file, lastelem);
    if (res < 0)
        return res;
    if (file == NULL || lastelem[0] != 0)
        return -E_FILE_EXISTS;

    if (ISDIR(file->f_mode))
        return -E_BAD_PATH;

    free_file_blocks(file);

    memset(file->f_name, 0, sizeof(file->f_name));
    file->f_size = 0;
    file->f_gid = 0;
    file->f_uid = 0;
    file->f_mode = 0;
    memset(file->f_direct, 0, sizeof(file->f_direct));
    file->f_indirect = 0;

    flush_block(file);
    free_block((uint64_t)file);

    flush_bitmap();
    return 0;
}

int
file_chmod(const char *path, uint32_t new_mode) {
    struct File *dir = NULL;
    struct File *file = NULL;
    char lastelem[MAXPATHLEN] = {};

    int res = walk_path(path, &dir, &file, lastelem);
    if (res < 0) return res;
    if (file == NULL || lastelem[0] != 0) return -E_FILE_EXISTS;

    int mask = IRWXU | IRWXG | IRWXO;
    file->f_mode = (file->f_mode & ~mask) | (new_mode & mask);

    return 0;
}

int 
file_chown(const char* path, uint64_t gid, uint64_t uid) {
    struct File *dir = NULL;
    struct File *file = NULL;
    char lastelem[MAXPATHLEN] = {};

    int res = walk_path(path, &dir, &file, lastelem);
    if (res < 0) return res;
    if (file == NULL || lastelem[0] != 0) return -E_FILE_EXISTS;

    file->f_gid = gid;
    file->f_uid = uid;
    return 0;
}