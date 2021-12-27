#include <inc/lib.h>
#include <user/user.h>
#include <inc/string.h>

user_t users[UID_MAX];
char buf[NBUFSIZ];
int r = 0;

void
gethome(char home[]) {
    int i;
    int cnt = 0;
    for (i = 0; cnt != 5; i++)
        if (buf[i] == ':')
            cnt++;
    i++;
    int j = 1;
    for (; buf[i] != ':'; i++, j++)
        home[j] = buf[i];
    home[j] = 0;
}

/*
 *  Returns 1 if user should be deleted else 0
 */
int
isdeluser(char* name) {
    int i;
    for (i = 0; buf[i] != ':'; i++)
        ;
    if (strncmp(name, buf, i))
        return 0;
    if (r) {
        char home[PATHLEN_MAX];
        home[0] = '/';
        gethome(home);
        const char* argv[3] = {"rm", home, NULL};
        printf("home = %s!\n", argv[1]);
        int r = spawn("rm", argv);
        if (r >= 0)
            wait(r);
    }
    return 1;
}

/*
 *  save position of : in line
 */
void
getargs(int iargs[]) {
    int k = 0;
    int i;
    for (i = 0; buf[i] != '\n'; i++) {
        if (buf[i] == ':') {
            buf[i] = 0;
            iargs[k++] = i;
        }
    }
    buf[i] = 0;
    iargs[k] = i;
}

/*
 *  copy userdata from /etc/passwd to users[]
 */
void
saveuser() {
    int iargs[6];
    getargs(iargs);
    uid_t userid = (uid_t)strtol(buf + iargs[1] + 1, NULL, 10);
    users[userid].u_uid = userid;
    gid_t groupid = (gid_t)strtol(buf + iargs[2] + 1, NULL, 10);
    users[userid].u_primgrp = groupid;
    strncpy(users[userid].u_comment, buf, iargs[0]);
    strncpy(users[userid].u_password, buf + iargs[0] + 1, iargs[1] - iargs[0]);
    strncpy(users[userid].u_home, buf + iargs[3] + 1, iargs[4] - iargs[3]);
    strncpy(users[userid].u_shell, buf + iargs[4] + 1, iargs[5] - iargs[4]);
}

/*
 *  fill new /etc/passwd with userdata
 */
void
printusers(int fd) {
    for (int i = 0; i < UID_MAX; i++) {
        if (users[i].u_uid != -1) {
            fprintf(fd, "%s:%s:%d:%d:%s:%s\n", users[i].u_comment, users[i].u_password, users[i].u_uid,
                    users[i].u_primgrp, users[i].u_home, users[i].u_shell);
        }
    }
}

/*
 *  init uids in users[]
 */
void
initusers() {
    for (int i = 0; i < UID_MAX; i++)
        users[i].u_uid = -1;
}

void
userdel(char* name) {
    int n;
    initusers();
    int fd = open("/etc/passwd", O_RDONLY);
    while ((n = getline(fd, buf, NBUFSIZ)) > 0) {
        if (!isdeluser(name))
            saveuser();
    }
    close(fd);
    fd = open("/etc/passwd", O_WRONLY | O_TRUNC);
    printusers(fd);
}

void
usage() {
    printf("usage:userdel [-r] LOGIN\n");
    exit();
}

void
umain(int argc, char** argv) {
    int i;
    struct Argstate args;
    argstart(&argc, argv, &args);
    if (argc == 1) {
        usage();
        return;
    }
    while ((i = argnext(&args)) >= 0) {
        switch (i) {
        case 'r':
            r = 1;
            if (argc == 1) {
                usage();
                return;
            }
            break;
        default:
            usage();
        }
    }
    userdel(argv[argc - 1]);
}
