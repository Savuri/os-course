#include <inc/lib.h>
#include <user/user.h>
#include <inc/string.h>

char* gid;
char* uid;

void
usage() {
    printf("usage:chmod UID[:GID] FILEPATH\n");
    exit();
}

void
umain(int argc, char** argv) {
    if(argc != 3)
        usage();
}