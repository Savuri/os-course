#include <inc/lib.h>
#include <user/user.h>
#include <inc/string.h>

char buf[NBUFSIZ];

void
usage() {
    printf("usage:groupadd GID\n");
    exit();
}

gid_t
readgids() {
    int i = 0;
    for (; buf[i] != ':'; i++)
        ;
    buf[i] = 0;
    return (gid_t)(strtol(buf, NULL, 10));
}

int
isgroupexists(gid_t gid) {
    int fd = open("/etc/group", O_RDONLY);
    if(fd < 0)
        return 0;
    while(getline(fd, buf, NBUFSIZ)) {
        if(gid == readgids())
            return 1;
    }
    close(fd);
    return 0;
}

void
umain(int argc, char** argv) {
    if (argc != 2) {
        usage();
        return;
    }
    int s = (int)strtol(argv[1], NULL, 10);
    if(s <= 0 || s >= 1024) {
        printf("GID should be > 0 and < 1024\n");
        return;
    }
    int res = isgroupexists((gid_t)strtol(argv[1], NULL, 10));
    int fd = open("/etc/group", O_WRONLY | O_CREAT | O_APPEND);
    if(!res)
        fprintf(fd, "%s:\n", argv[1]);
    else
        printf("GID = %s is already in use\n", argv[1]);
}