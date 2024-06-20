#include <inc/lib.h>

const char* directory = "/new_folder";
const char* file1     = "/new_folder/file1";
const char* file2     = "/new_folder/file2";
const char* file3     = "/new_folder/file3";

void test_mkdir() {
    cprintf("\n==========================\nSTART TEST MKDIR\n==========================\n");
    int res = 0;

    res = mkdir(directory);
    cprintf("MKDIR TEST: During mkdir \"%s\" <%i>(%d)\n", directory, res, res);
 
    int fd = open(file1, O_RDWR | O_CREAT);
    cprintf("MKDIR TEST: open \"%s\" <%i>(%d)\n", file1, fd, fd);
    cprintf("fd after open \"%s\" %d\n", file1, fd);

    close(fd);
    cprintf("\n==========================\nEND TEST MKDIR\n==========================\n");
}

void test_remove() {
    cprintf("\n==========================\nSTART TEST REMOVE\n==========================\n");
    int res = 0;

    res = mkdir(directory);
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

void test_getdents() {
    cprintf("\n==========================\nSTART TEST GETDENTS\n==========================\n");
    int res = 0;

    res = mkdir(directory);
    cprintf("GETDENTS TEST: During mkdir \"%s\" <%i>(%d)\n", directory, res, res);

    int fd1 = open(file1, O_RDWR | O_CREAT);
    int fd2 = open(file2, O_RDWR | O_CREAT);
    int fd3 = open(file3, O_RDWR | O_CREAT);
    cprintf("REMOVE TEST: open \"%s\" <%i>(%d)\n", file1, fd1, fd1);
    cprintf("REMOVE TEST: open \"%s\" <%i>(%d)\n", file2, fd2, fd2);
    cprintf("REMOVE TEST: open \"%s\" <%i>(%d)\n", file3, fd3, fd3);
    close(fd1);
    close(fd2);
    close(fd3);

    struct File files[3];
    res = getdents(directory, files, 2);
    cprintf("REMOVE TEST: getdents \"%s\", 2 <%i>(%d)\n", directory, res, res);
    cprintf("REMOVE TEST: file[0] = <%s>\n", files[0].f_name);
    cprintf("REMOVE TEST: file[1] = <%s>\n", files[1].f_name);
    res = getdents(directory, files, 3);
    cprintf("REMOVE TEST: getdents \"%s\", 3 <%i>(%d)\n", directory, res, res);
    cprintf("REMOVE TEST: file[0] = <%s>\n", files[0].f_name);
    cprintf("REMOVE TEST: file[1] = <%s>\n", files[1].f_name);
    cprintf("REMOVE TEST: file[1] = <%s>\n", files[2].f_name);


    cprintf("\n==========================\nEND TEST GETDENTS\n==========================\n");
}

void
umain(int argc, char **argv) {
    test_mkdir();
    test_remove();
    test_getdents();
}
