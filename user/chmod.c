#include <inc/lib.h>
#include <user/user.h>
#include <inc/string.h>

void
usage() {
    printf("usage:chmod MASK(NNNN) FILEPATH\n");
    exit();
}

void
umain(int argc, char** argv) {
    if (argc != 3)
        usage();
    int arg = strtol(argv[1], NULL, 8);
    permission_t mask = 0x0FFF & arg;
    if (chmod(argv[2], mask))
        printf("Bad chmod\n");
}