#include <inc/lib.h>

const char *msg = "This is the NEW message of the day!\n\n";

#define FVA ((struct Fd *)0xA000000)

static void
test_read_only() {
    int64_t r;
    char buf[512];

    int64_t f1 = open("/read-only", O_RDONLY);
    if (f1 >= 0) {
        cprintf("1.1 OK\n");

        memset(buf, 0, sizeof(buf));
        if ((r = readn(f1, buf, sizeof(buf))) < 0) {
            cprintf("1.2 BAD\n");
        } else {
            cprintf("1.2 OK\n");
        }

        if ((r = write(f1, buf, sizeof(buf))) < 0) {
            cprintf("1.3 OK\n");
        } else {
            cprintf("1.3 BAD\n");
        }

        close(f1);
    } else {
        cprintf("1.1 BAD\n");
    }

    int64_t f2 = open("/read-only", O_WRONLY);
    if (f2 >= 0) {
        cprintf("2. BAD\n");
        close(f2);
    } else {
        cprintf("2. OK\n");
    }
}


static void
test_write_only() {
    int64_t r;
    char buf[512];

    int64_t f1 = open("/write-only", O_WRONLY);
    if (f1 >= 0) {
        cprintf("2.1 OK\n");

        memset(buf, 0, sizeof(buf));
        if ((r = write(f1, buf, sizeof(buf))) < 0) {
            cprintf("2.2 BAD\n");
        } else {
            cprintf("2.2 OK\n");
        }

        memset(buf, 0, sizeof(buf));
        if ((r = write(f1, buf, sizeof(buf))) < 0) {
            cprintf("2.2 BAD\n");
        } else {
            cprintf("2.2 OK\n");
        }

        close(f1);
    } else {
        cprintf("2.1 BAD\n");
    }

    int64_t f2 = open("/write-only", O_RDONLY);
    if (f2 >= 0) {
        cprintf("2. BAD\n");
        close(f2);
    } else {
        cprintf("2. OK\n");
    }
}

void
umain(int argc, char **argv) {
    cprintf("Test read-only:\n");
    test_read_only();

    cprintf("Test write-only:\n");
    test_write_only();

    cprintf("finish\n");
}
