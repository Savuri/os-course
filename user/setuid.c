#include <inc/lib.h>


void
umain(int argc, char **argv) {
    if (argc != 2) {
        return;
    }

    long i;
    i = strtol(argv[1], NULL, 10);
    printf("Going switch current uid to %ld\n", i);

    struct Ucred ucred = envs[ENVX(thisenv->env_id)].env_ucred;
    printf("BEFORE: cur proc cred uid=%u, euid=%u, gid=%u, egid=%u\n", ucred.cr_ruid, ucred.cr_uid, ucred.cr_rgid, ucred.cr_gid);


    if (sys_setuid(i) < 0) {
        printf("Error in syscall");
    }

    ucred = envs[ENVX(thisenv->env_id)].env_ucred;
    printf("AFTER: cur proc cred uid=%u, euid=%u, gid=%u, egid=%u\n", ucred.cr_ruid, ucred.cr_uid, ucred.cr_rgid, ucred.cr_gid);
}
