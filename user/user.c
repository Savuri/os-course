#include <inc/lib.h>
#include <user/user.h>
#include <inc/string.h>

char buf[256];

uid_t
readuid() {
    int i;
    int cnt = 0;
    for (i = 0; cnt != 2; i++)
        if (buf[i] == ':')
            cnt++;
    i++;
    int l = i;
    int r = i;
    for (; buf[i] != ':'; i++, r++)
        ;
    buf[r] = 0;
    return (uid_t)(strtol(buf[l], NULL, 10));
}

gid_t
readgid() {
    int i;
    int l = i;
    int r = i;
    for (; buf[i] != ':'; i++, r++)
        ;
    buf[r] = 0;
    return (gid_t)(strtol(buf[l], NULL, 10));
}

int
getline(int fd) {
    int i = 0;
    int r = read(fd, &buf[i], 1);
    if (!r)
        return 0;
    while (buf[i] != '\n') {
        i++;
        r = read(fd, &buf[i], 1);
        if (!r)
            return 0;
    }
    i++;
    buf[i] = 0;
    return 1;
}

int
isuserexist(uid_t uid) {
    int fd = open("/etc/passwd", O_RDONLY);
    if(fd < 0)
        return 0;
    while(getline(fd)) {
        if(uid == readuid())
            return 1;
    }
    return 0;
}

int
isgroupexist(gid_t gid) {
    int fd = open("/etc/groups", O_RDONLY);
    if(fd < 0)
        return 0;
    while(getline(fd)) {
        if(gid == readgid())
            return 1;
    }
    return 0;
}