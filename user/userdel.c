#include <inc/lib.h>
#include <user/user.h>
#include <inc/string.h>

user_t users[UID_MAX];
char buf[256];
int r = 0;

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
isdeluser(char* name) {
    int i;
    for (i = 0; buf[i] != ':'; i++)
        ;
    if (strncmp(name, buf, i))
        return 0;
    if (r)
        ; //delete userhome
    return 1;
}

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

void
printusers(int fd) {
    for (int i = 0; i < UID_MAX; i++) {
        if (users[i].u_uid != -1) {
            fprintf(fd, "%s:%s:%d:%d:%s:%s\n", users[i].u_comment, users[i].u_password, users[i].u_uid,
                    users[i].u_primgrp, users[i].u_home, users[i].u_shell);
        }
    }
}

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
    while ((n = getline(fd)) != 0) {
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
            if (argc == 2) {
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