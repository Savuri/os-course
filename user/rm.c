#include "inc/lib.h"

void
umain(int argc, char *argv[]) {
    if (argc != 2) {
        cprintf("Usage: rm PATH\n");
        exit();
    }

    int ret = remove(argv[1]);
    if (ret < 0) {
        printf("%i\n", ret);
    }
}
