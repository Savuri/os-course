#include <inc/lib.h>

static const char *file_name = "file";
// static const char *dir_name = "dir";

void
umain(int argc, char *argv[]) {
    if (sys_geteuid()) {
        cprintf("ERROR: This test should be run by root");
        return;
    }

    char cur_dir[MAXPATHLEN];
    sys_setenvcurpath("/");
    sys_getenvcurpath(cur_dir, 0);
    if (strcmp(cur_dir, "/")) {
        cprintf("ERROR: unexpected current dir\n");
        cprintf("Expected: /\nProvided: %s\n", cur_dir);
        return;
    }

    int fd1;
    if ((fd1 = open(file_name, O_CREAT)) < 0) {
        cprintf("create %s fail: %d\n", file_name, fd1);
        // return;
    } else {
        cprintf("%d\n", fd1);
    }

    int fd2;
    if ((fd2 = open(file_name, O_RDONLY)) < 0) {
        cprintf("create %s fail: %d\n", file_name, fd2);
        // return;
    } else {
        cprintf("%d\n", fd2);
    }

    // int res;
    // if ((res = remove(file_name))) {
    //     cprintf("oops: we managed to remove open file\n");
    // }

    if (close(fd1) < 0) {
        cprintf("close %s fail: %d\n", file_name, fd1);
    } else {
        cprintf("%d\n", fd1);
    }

    if (close(fd2) < 0) {
        cprintf("close %s fail: %d\n", file_name, fd2);
    } else {
        cprintf("%d\n", fd2);
    }


    cprintf("OK\n");
}