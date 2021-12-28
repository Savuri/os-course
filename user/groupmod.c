#include <inc/lib.h>
#include <user/user.h>
#include <inc/string.h>

char buf[NBUFSIZ];
char* gid;
char* uid;
int grnum = 0;

typedef struct group {
    char line[256];
} group_t;

group_t group[1024];

void
usage() {
    printf("usage:groupmod GID UID\n");
    exit();
}

void
groupmod() {
    int fd = open("/etc/group", O_RDONLY);
    while (getline(fd, buf, NBUFSIZ)) {
        int i = 0;
        while (buf[i] != ':')
            i++;
        if (!strncmp(buf, gid, i)) {
            while (buf[i])
                i++;
            buf[i] = ':';
            i++;
            strncpy(buf + i, uid, strlen(uid));
            i += strlen(uid);
            buf[i] = ':';
            i++;
            buf[i] = '\n';
        }
        while (buf[i] != '\n')
            i++;
        buf[i] = 0;
        strncpy(group[grnum].line, buf, i);
        grnum++;
    }
    close(fd);
    fd = open("/etc/group", O_WRONLY | O_TRUNC);
    for (int i = 0; i < grnum; i++)
        fprintf(fd, "%s\n", group[i].line);
    close(fd);
}

void
umain(int argc, char** argv) {
    if (argc != 3)
        usage();
    gid = argv[1];
    uid = argv[2];
    if (strtol(uid, NULL, 10) >= 1024 || strtol(uid, NULL, 10) <= 0) {
        printf("UID should be > 0 and < 1024\n");
        exit();
    }
    if (strtol(gid, NULL, 10) <= 0 || strtol(gid, NULL, 10) >= 1024) {
        printf("GID should be > 0 and < 1024\n");
        exit();
    }
    if (!isgroupexist(strtol(gid, NULL, 10))) {
        int fd = open("/etc/group", O_WRONLY | O_CREAT | O_APPEND);
        fprintf(fd, "%s:%s\n", gid, uid);
        close(fd);
    } else
        groupmod();
}