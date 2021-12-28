#include <inc/lib.h>

static const char *MY_TMP_DIR = "/my_tmp_dir";
const permission_t RWX______ = 0b111000000;
const permission_t ___RWX___ = 0b000111000;
const permission_t ______RWX = 0b000000111;

const int TEST_STATUS_SIZE = 12;

enum Constants {
    ERROR = 0,
    SUCCESS = 1,
};

static int
prepare() {
    char cur_dir[MAXPATHLEN];

    sys_setenvcurpath("/");
    sys_getenvcurpath(cur_dir, 0);
    if (strcmp(cur_dir, "/")) {
        cprintf("ERROR: unexpected current dir\n");
        cprintf("Expected: /\nProvided: %s\n", cur_dir);
        return -1;
    }

    int ret = open(MY_TMP_DIR, O_MKDIR);
    if (ret < 0) {
        cprintf("ERROR: fail to create %s\n", MY_TMP_DIR);
        return -1;
    }

    if (chown(MY_TMP_DIR, 1000)) {
        cprintf("fail to change owner of %s\n", MY_TMP_DIR);
        return -1;
    }

    if (chgrp(MY_TMP_DIR, 1000)) {
        cprintf("fail to change group of %s\n", MY_TMP_DIR);
        return -1;
    }

    if (chdir(MY_TMP_DIR)) {
        cprintf("fail to change current dir on %s\n", MY_TMP_DIR);
        return -1;
    }

    return 0;
}

static void
finish() {
    int ret = remove(MY_TMP_DIR);
    if (ret != 0) {
        cprintf("ERROR: fail to remove %s\n", MY_TMP_DIR);
    }
}

static void
create_and_remove(const char *who) {
    int res;
    if (!(res = open(who, O_CREAT))) {
        cprintf("%s: create file success: %d\n", who, res);
    } else {
        cprintf("%s: create file fail: %i\n", who, res);
    }

    if (!(res = close(res))) {
        cprintf("%s: close file success: %d\n", who, res);
    } else {
        cprintf("%s: close file fail: %i\n", who, res);
    }

    if (!(res = remove(who))) {
        cprintf("%s: remove file success: %d\n", who, res);
    } else {
        cprintf("%s: remove file fail: %i\n", who, res);
    }

    if (!(res = open(who, O_MKDIR))) {
        cprintf("%s: create dir success: %d\n", who, res);
    } else {
        cprintf("%s: create dir fail: %i\n", who, res);
    }

    if (!(res = close(res))) {
        cprintf("%s: close dir success: %d\n", who, res);
    } else {
        cprintf("%s: close dir fail: %i\n", who, res);
    }

    if (!(res = remove(who))) {
        cprintf("%s: remove dir success: %d\n", who, res);
    } else {
        cprintf("%s: remove dir fail: %i\n", who, res);
    }
}

static void
set_creds(const char *who) {
    if (!strcmp(who, "owner")) {
        sys_seteuid(1000);
        sys_setegid(1001);
    } else if (!strcmp(who, "group")) {
        sys_seteuid(1001);
        sys_setegid(1000);
    } else if (!strcmp(who, "other")) {
        sys_seteuid(1001);
        sys_setegid(1001);
    } else {
        cprintf("fail to set creds\n");
    }
}

static void
test(const char *who) {
    envid_t pid = fork();
    if (!pid) {
        sys_setenvcurpath(MY_TMP_DIR);
        set_creds(who);
        create_and_remove(who);
        exit();
    }
    wait(pid);
}

static void
run_test(permission_t perm) {
    cprintf("\n");
    test("owner");
    cprintf("\n");
    test("group");
    cprintf("\n");
    test("other");
}

void
umain(int argc, char **argv) {
    cprintf("Prepare for the test\n");
    if (prepare()) {
        finish();
        return;
    }

    if (chmod(MY_TMP_DIR, RWX______) != 0) {
        cprintf("ERROR: chmod fail before Test1");
        finish();
    }
    cprintf("Test1: rwx------\n");
    run_test(RWX______);


    if (chmod(MY_TMP_DIR, ___RWX___) != 0) {
        cprintf("ERROR: chmod fail before Test2");
        finish();
    }
    cprintf("\n\nTest2: ---rwx---\n");
    run_test(___RWX___);

    if (chmod(MY_TMP_DIR, ______RWX) != 0) {
        cprintf("ERROR: chmod fail before Test3");
        finish();
    }
    cprintf("\n\nTest3: ------rwx\n");
    run_test(______RWX);

    finish();
}
