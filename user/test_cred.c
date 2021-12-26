#include <inc/lib.h>

void
umain(int argc, char *argv[]) {
    printf("UID= %d\nEUID=%d\nGID= %d\nEGID=%d\n", sys_getuid(), sys_geteuid(), sys_getgid(), sys_getegid());
    return;
}
