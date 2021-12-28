#include <inc/lib.h>


void
umain(int argc, char **argv) {
    printf("uid=%u, euid=%u, gid=%u, egid=%u\n", sys_getuid(), sys_geteuid(), sys_getgid(), sys_getegid());
}
