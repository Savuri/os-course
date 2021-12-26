#include "inc/lib.h"

void
umain(int argc, char *argv[]) {
    int ret = open(argv[1], O_MKDIR);
    if (ret < 0) {
        printf("%i\n", ret);
    }
}
