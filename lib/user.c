#include <inc/lib.h>
#include <user/user.h>
#include <inc/string.h>

static char user_file_buf[NBUFSIZ];

static uid_t
readuid() {
    int i;
    int cnt = 0;
    for (i = 0; cnt != 2; i++)
        if (user_file_buf[i] == ':')
            cnt++;
    i++;
    int l = i;
    for (; user_file_buf[i] != ':'; i++)
        ;
    user_file_buf[i] = 0;
    printf("%s\n", user_file_buf + l - 1);
    return (uid_t)(strtol(user_file_buf + l - 1, NULL, 10));
}

static gid_t
readgid() {
    int i = 0;
    for (; user_file_buf[i] != ':'; i++)
        ;
    user_file_buf[i] = 0;
    return (gid_t)(strtol(user_file_buf, NULL, 10));
}

int
isuserexist(uid_t uid) {
    printf("uid=%d!\n", uid);
    int fd = open("/etc/passwd", O_RDONLY);
    if (fd < 0)
        return 0;
    while (getline(fd, user_file_buf, NBUFSIZ) == 1) {
        if (uid == readuid())
            return 1;
    }
    close(fd);
    return 0;
}

int
isgroupexist(gid_t gid) {

    int fd = open("/etc/group", O_RDONLY);
    if (fd < 0)
        return 0;
    while (getline(fd, user_file_buf, NBUFSIZ)) {
        if (gid == readgid())
            return 1;
    }
    close(fd);
    return 0;
}
