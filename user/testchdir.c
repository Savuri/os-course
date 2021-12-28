#include <inc/lib.h>

static void
test_chdir(const char *dir, const char *expected) {
    chdir(dir); // not check the return value

    char cur_dir[MAXPATHLEN];
    sys_getenvcurpath(cur_dir, 0);
    if (strcmp(cur_dir, expected)) {
        cprintf("ERROR. Expected: %s    Provided: %s\n", expected, cur_dir);
        return;
    }
}

void
umain(int argc, char **argv) {
    char cur_dir[MAXPATHLEN];

    if (sys_geteuid()) {
        cprintf("ERROR: This test should be run by root");
        return;
    }

    cprintf("I am root\n");

    sys_setenvcurpath("/");
    sys_getenvcurpath(cur_dir, 0);
    if (strcmp(cur_dir, "/")) {
        cprintf("ERROR: unexpected current dir\n");
        cprintf("Expected: /\nProvided: %s\n", cur_dir);
        return;
    }

    cprintf("Now i am root\n");

    test_chdir("/etc", "/etc");
    test_chdir("/", "/");
    test_chdir("/etc/", "/etc");
    test_chdir("/", "/");
    test_chdir("etc/", "/etc");
    test_chdir("/", "/");
    test_chdir("etc", "/etc");

    test_chdir("/", "/");
    test_chdir("./etc", "/etc");
    test_chdir("/", "/");
    test_chdir("../etc", "/etc");
    test_chdir("/", "/");
    test_chdir("../etc/.", "/etc");
    test_chdir("/", "/");
    test_chdir("./etc/.", "/etc");

    test_chdir("/etc", "/etc");
    test_chdir("etc", "/etc"); // there is no etc in /etc
    test_chdir("/", "/");
    test_chdir("etc/etc", "/"); // there is no etc in /etc
    test_chdir("/", "/");
    test_chdir("/1/2/3/4", "/");

    test_chdir("/root", "/root");
    test_chdir("hello", "/root"); // hello is regular file


    cprintf("Try to become user\n");
    envid_t pid = fork();
    if (pid) {
        wait(pid);
        cprintf("Done\n");
        return;
    }

    test_chdir("/home/user", "/home/user");
    if (sys_setegid(1000)) {
        cprintf("ERROR: can't set egid\n");
        return;
    }
    if (sys_seteuid(1000)) {
        cprintf("ERROR: can't set euid\n");
        return;
    }

    cprintf("Now i am user\n");
    test_chdir(".", "/home/user");
    test_chdir("../..", "/");
    test_chdir("home", "/home");
    test_chdir("../root", "/home"); // permission denied
}
