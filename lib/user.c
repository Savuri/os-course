#include <inc/lib.h>
#include <user/user.h>
#include <inc/string.h>

static char user_file_buf[256];

static uid_t
readuid() {
    int i;
    int cnt = 0;
    for (i = 0; cnt != 2; i++)
        if (user_file_buf[i] == ':')
            cnt++;
    i++;
    int l = i;
    int r = i;
    for (; user_file_buf[i] != ':'; i++, r++)
        ;
    user_file_buf[r] = 0;
    return (uid_t)(strtol(&user_file_buf[l], NULL, 10));
}

static gid_t
readgid() {
    int i = 0;
    int l = i;
    for (; user_file_buf[i] != ':'; i++)
        ;
    user_file_buf[i] = 0;
    return (gid_t)(strtol(&user_file_buf[l], NULL, 10));
}

int
isuserexist(uid_t uid) {
    int fd = open("/etc/passwd", O_RDONLY);
    if (fd < 0)
        return 0;
    while (getline(fd, user_file_buf, 256) == 1) {
        if (uid == readuid())
            return 1;
    }
    return 0;
}

int
isgroupexist(gid_t gid) {

    int fd = open("/etc/groups", O_RDONLY);
    if (fd < 0)
        return 0;
    while (getline(fd, user_file_buf, 256)) {
        if (gid == readgid())
            return 1;
    }
    return 0;
}
