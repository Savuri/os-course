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

    chown(MY_TMP_DIR, 1);
    chgrp(MY_TMP_DIR, 1);

    chdir(MY_TMP_DIR);

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
create_and_remove(const char *who, char test_status[TEST_STATUS_SIZE]) {
    static int i = 0;
    i %= TEST_STATUS_SIZE;

    if (!open(who, O_CREAT)) {
        test_status[i] = SUCCESS;
        cprintf("!!!create file success\n\n");
    } else {
        test_status[i] = ERROR;
    }
    ++i;

    if (!remove(who)) {
        test_status[i] = SUCCESS;
    } else {
        test_status[i] = ERROR;
    }
    ++i;

    if (!open(who, O_MKDIR)) {
        test_status[i] = SUCCESS;
        cprintf("!!!create dir success\n\n");
    } else {
        test_status[i] = ERROR;
    }
    ++i;

    if (!remove(who)) {
        test_status[i] = SUCCESS;
    } else {
        test_status[i] = ERROR;
    }
}

static void
make_expected_status(char status[TEST_STATUS_SIZE], permission_t perm) {
    memset(status, ERROR, TEST_STATUS_SIZE);

    if (perm & 1) {
        memset(status + 8, SUCCESS, 4);
    }
    if (perm & 0b1000) {
        memset(status + 4, SUCCESS, 4);
    }
    if (perm & 0b1000000) {
        memset(status, SUCCESS, 4);
    }
}

static void
compare(const char provided[TEST_STATUS_SIZE],
        const char expected[TEST_STATUS_SIZE],
        permission_t permision) {
    for (int i = 0; i < TEST_STATUS_SIZE; ++i) {
        if (provided[i] == expected[i]) {
            continue;
        }

        switch (i / 4) {
        case 0:
            cprintf("Owner fail: ");
            break;
        case 1:
            cprintf("Group fail: ");
            break;
        case 2:
            cprintf("Other fail: ");
            break;
        }

        switch (i % 4) {
        case 0:
            cprintf("file create\n");
            break;
        case 1:
            cprintf("file remove\n");
            break;
        case 2:
            cprintf("dir create\n");
            break;
        case 3:
            cprintf("dir remove\n");
            break;
        }
    }
}

static void
set_creds(const char *who) {
    if (!strcmp(who, "owner")) {
        sys_seteuid(1);
    } else if (!strcmp(who, "group")) {
        sys_seteuid(2);
        sys_setegid(1);
    } else if (!strcmp(who, "other")) {
        sys_seteuid(2);
        sys_setegid(2);
    } else {
        cprintf("fail to set creds\n");
    }
}

static void
test(const char *who, char test_status[TEST_STATUS_SIZE]) {
    int p[2];
    if (pipe(p) < 0) {
        cprintf("pipe fail: %s\n", who);
        return;
    }

    int pid = fork();
    if (!pid) {
        sys_setenvcurpath("/");
        close(p[0]);
        set_creds(who);
        create_and_remove(who, test_status);
        write(p[1], test_status, TEST_STATUS_SIZE);
        close(p[1]);
        exit();
    }
    close(p[1]);
    read(p[0], test_status, TEST_STATUS_SIZE);
    close(p[0]);
    wait(pid);
}

static void
run_test(permission_t perm) {
    char test_status[TEST_STATUS_SIZE];


    cprintf("It is owner\n");
    test("owner", test_status);
    cprintf("It is group\n");
    test("group", test_status);
    cprintf("It is other\n");
    test("other", test_status);

    char expected_status[TEST_STATUS_SIZE];
    make_expected_status(expected_status, perm);

    compare(test_status, expected_status, perm);
}

void
umain(int argc, char **argv) {
    int res;

    if (prepare()) {
        finish();
        return;
    }

    if ((res = chmod(MY_TMP_DIR, RWX______)) != 0) {
        cprintf("ERROR: chmod fail before Test1");
        finish();
    }
    cprintf("Test1: rwx------\n");
    run_test(RWX______);


    if ((res = chmod(MY_TMP_DIR, ___RWX___)) != 0) {
        cprintf("ERROR: chmod fail before Test2");
        finish();
    }
    cprintf("Test2: ---rwx---\n");
    run_test(___RWX___);

    if ((res = chmod(MY_TMP_DIR, ______RWX)) != 0) {
        cprintf("ERROR: chmod fail before Test3");
        finish();
    }
    cprintf("Test3: ------rwx\n");
    run_test(______RWX);

    finish();
}
