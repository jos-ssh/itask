#include <inc/lib.h>

const char* directory = "/new_folder";
const char* file1     = "/new_folder/file1";
const char* file2     = "/new_folder/file2";
const char* file3     = "/new_folder/file3";

void test_mkdir() {
    cprintf("\n==========================\nSTART TEST MKDIR\n==========================\n");
    int res = 0;

    res = mkdir(directory, 0);
    cprintf("MKDIR TEST: During mkdir \"%s\" <%i>(%d)\n", directory, res, res);
 
    int fd = open(file1, O_RDWR | O_CREAT);
    cprintf("MKDIR TEST: open \"%s\" <%i>(%d)\n", file1, fd, fd);

    close(fd);
    cprintf("\n==========================\nEND TEST MKDIR\n==========================\n");
}

void test_remove() {
    cprintf("\n==========================\nSTART TEST REMOVE\n==========================\n");
    int res = 0;

    res = mkdir(directory, 0);
    cprintf("REMOVE TEST: During mkdir \"%s\" <%i>(%d)\n", directory, res, res);
 
    int fd1 = open(file1, O_RDWR | O_CREAT);
    int fd2 = open(file2, O_RDWR | O_CREAT);
    int fd3 = open(file3, O_RDWR | O_CREAT);
    cprintf("REMOVE TEST: open \"%s\" <%i>(%d)\n", file1, fd1, fd1);
    cprintf("REMOVE TEST: open \"%s\" <%i>(%d)\n", file2, fd2, fd2);
    cprintf("REMOVE TEST: open \"%s\" <%i>(%d)\n", file3, fd3, fd3);
    close(fd1);
    close(fd2);
    close(fd3);
    
    res = remove(file1);
    cprintf("REMOVE TEST: remove \"%s\" <%i>(%d)\n", file1, res, res);
    fd1 = open(file1, O_RDWR);
    cprintf("REMOVE TEST: open \"%s\" <%i>(%d)\n", file1, fd1, fd1);

    res = remove(directory);
    cprintf("REMOVE TEST: remove \"%s\" <%i>(%d)\n", directory, res, res);
    fd2 = open(file2, O_RDWR);
    cprintf("REMOVE TEST: open \"%s\" <%i>(%d)\n", file2, fd2, fd2);
    fd3 = open(file3, O_RDWR);
    cprintf("REMOVE TEST: open \"%s\" <%i>(%d)\n", file3, fd3, fd3);
    
    cprintf("\n==========================\nEND TEST REMOVE\n==========================\n");
}

void test_getdents_small_number()
{
    cprintf("\n==========================\nSTART TEST GETDENTS SMALL N\n==========================\n");
    int res = 0;

    res = mkdir(directory, 0);
    cprintf("GETDENTS SMALL N TEST: During mkdir \"%s\" <%i>(%d)\n", directory, res, res);

    int fd1 = open(file1, O_RDWR | O_CREAT);
    int fd2 = open(file2, O_RDWR | O_CREAT);
    int fd3 = open(file3, O_RDWR | O_CREAT);
    cprintf("GETDENTS SMALL N TEST: open \"%s\" <%i>(%d)\n", file1, fd1, fd1);
    cprintf("GETDENTS SMALL N TEST: open \"%s\" <%i>(%d)\n", file2, fd2, fd2);
    cprintf("GETDENTS SMALL N TEST: open \"%s\" <%i>(%d)\n", file3, fd3, fd3);
    close(fd1);
    close(fd2);
    close(fd3);

    struct FileInfo files[3];
    res = getdents(directory, files, 2);
    cprintf("GETDENTS SMALL N TEST: getdents \"%s\", 2 <%i>(%d)\n", directory, res, res);
    cprintf("GETDENTS SMALL N TEST: file[0] = <%s>\n", files[0].f_name);
    cprintf("GETDENTS SMALL N TEST: file[1] = <%s>\n", files[1].f_name);
    res = getdents(directory, files, 3);
    cprintf("GETDENTS SMALL N TEST: getdents \"%s\", 3 <%i>(%d)\n", directory, res, res);
    cprintf("GETDENTS SMALL N TEST: file[0] = <%s>\n", files[0].f_name);
    cprintf("GETDENTS SMALL N TEST: file[1] = <%s>\n", files[1].f_name);
    cprintf("GETDENTS SMALL N TEST: file[1] = <%s>\n", files[2].f_name);

    cprintf("\n==========================\nEND TEST GETDENTS SMALL N\n==========================\n");
}

void test_getdents_big_number()
{
    const uint32_t file_number = 22;
    
    cprintf("\n==========================\nSTART TEST GETDENTS BIG N\n==========================\n");
    int res = 0;

    res = mkdir(directory, 0);
    if (res < 0)
        cprintf("GETDENTS BIG N TEST: During mkdir \"%s\" <%i>(%d)\n", directory, res, res);

    char file_path[MAXPATHLEN];
    for (int i = 0; i < file_number; i++)
    {
        snprintf(file_path, MAXPATHLEN, "%s/file%d", directory, i);
        res = open(file_path, O_RDWR | O_CREAT);
        if (res < 0)
            cprintf("GETDENTS BIG N TEST: open \"%s\" <%i>(%d)\n", file_path, res, res);
        close(res);
    }

    struct FileInfo files[file_number];
    res = getdents(directory, files, file_number);
    for (int i = 0; i < file_number; i++)
        cprintf("GETDENTS BIG N TEST: [%d] <%s>\n", i, files[i].f_name);

    cprintf("\n==========================\nEND TEST GETDENTS BIG N\n==========================\n");
}

void test_getdents() {
    test_getdents_small_number();
    test_getdents_big_number();
}

void
umain(int argc, char **argv) {
    test_mkdir();
    test_remove();
    test_getdents();
}
