#include <inc/lib.h>


static int
is_root(const char *who) {
    return !strcmp(who, "root");
}

static void
test_read_only(const char *who) {
    int64_t r;
    char buf[512];

    int64_t f1 = open("/read-only", O_RDONLY);
    if (f1 < 0) {
        cprintf("ERROR: %s couldn't open /read-only file with O_RDONLY\n", who);
    } else {
        if ((r = readn(f1, buf, sizeof(buf))) < 0) {
            cprintf("ERROR: %s couldn't read from read-only file opened with O_RDONLY\n", who);
        }

        if ((r = write(f1, buf, sizeof(buf))) >= 0) {
            cprintf("ERROR: %s could write to read-only file opened with O_RDONLY\n", who);
        }

        close(f1);
    }

    int64_t f2 = open("/read-only", O_WRONLY);
    if (f2 < 0) {
        if (is_root(who)) {
            cprintf("ERROR: root couldn't open /read-only file with O_WRONLY\n");
        }
    } else {
        if (!is_root(who)) {
            cprintf("ERROR: %s could open /read-only file with O_WRONLY\n", who);
        } else if ((r = write(f2, buf, sizeof(buf))) < 0) {
            cprintf("ERROR: root couldn't write to read-only file opened with O_WRONLY\n");
        }

        close(f2);
    }
}

static void
test_write_only(const char *who) {
    int64_t r;
    char buf[512];

    int64_t f1 = open("/write-only", O_WRONLY);
    if (f1 < 0) {
        cprintf("ERROR: %s couldn't open /write-only file with O_WRONLY\n", who);
    } else {
        memset(buf, 0, sizeof(buf));
        if ((r = write(f1, buf, sizeof(buf))) < 0) {
            cprintf("ERROR: %s couldn't write to write-only file opened with O_WRONLY\n", who);
        }

        memset(buf, 0, sizeof(buf));
        if ((r = readn(f1, buf, sizeof(buf))) >= 0) {
            cprintf("ERROR: %s could read from write-only file opened with O_WRONLY\n", who);
        }

        close(f1);
    }

    int64_t f2 = open("/write-only", O_RDONLY);
    if (f2 < 0) {
        if (is_root(who)) {
            cprintf("ERROR: root couldn't open /write-only file with O_RDONLY\n");
        }
    } else {
        if (!is_root(who)) {
            cprintf("ERROR: %s could open /write-only file with O_RDONLY\n", who);
        } else if ((r = readn(f2, buf, sizeof(buf))) < 0) {
            cprintf("ERROR: root couldn't read from read-only file opened with O_RDONLY\n");
        }

        close(f2);
    }
}

static void
test(const char *who) {
    cprintf("\n---------%s: test /read-only----------------\n", who);
    test_read_only(who);

    cprintf("\n---------%s: test /write-only----------------\n", who);
    test_write_only(who);
}

void
umain(int argc, char **argv) {
    envid_t pid;
    (void)pid;
    test("root");

    pid = fork();
    if (!pid) {
        sys_seteuid(1);
        sys_setegid(1);
        test("owner");
        exit();
    }
    wait(pid);

    pid = fork();
    if (!pid) {
        sys_seteuid(2);
        sys_setegid(1);
        test("group");
        exit();
    }
    wait(pid);

    pid = fork();
    if (!pid) {
        sys_seteuid(2);
        sys_setegid(2);
        test("other");
        exit();
    }
    wait(pid);
}
