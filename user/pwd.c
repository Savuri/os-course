#include "inc/lib.h"

static char buf[MAXPATHLEN];

void
umain(int argc, char *argv[]) {
    if (argc != 1) {
        cprintf("Usage: pwd\n");
        exit();
    }
    sys_getenvcurpath(buf, 0);
    printf("%s\n", buf);
}