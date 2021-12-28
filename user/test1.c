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

    int fd;
    if ((fd = open(file_name, O_CREAT))) {
        cprintf("create %s fail: %i\n", file_name, fd);
    }
    close(fd);

}