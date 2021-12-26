#include "inc/lib.h"

void
umain(int argc, char *argv[]) {
    int ret = remove(argv[1]);
    if (ret < 0) {
        printf("%i\n", ret);
    }
}
