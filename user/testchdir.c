#include <inc/lib.h>

static void
test_chdir(const char *dir, const char *expected) {
    chdir(dir);
    
    char cur_dir[MAXPATHLEN];
    sys_getenvcurpath(cur_dir, 0);
    if (strcmp(cur_dir, expected)) {
        cprintf("ERROR:\nExpected: /\nProvided: %s", cur_dir);
        return;
    }
}

void
umain(int argc, char **argv) {
    char cur_dir[MAXPATHLEN];

    sys_setenvcurpath("/");
    sys_getenvcurpath(cur_dir, 0);
    if (strcmp(cur_dir, "/")) {
        cprintf("ERROR: unexpected current dir\n");
        cprintf("Expected: /\nProvided: %s", cur_dir);
        return;
    }

    test_chdir("/etc", "/etc");
    sys_setenvcurpath("/");
    test_chdir("etc", "/etc");
}
