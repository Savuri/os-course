#include <inc/lib.h>
#include <user/user.h>
#include <inc/string.h>
#include <inc/crypt.h>
#include <inc/base64.h>

int flag[256];
int uids[UID_MAX];
int gids[UID_MAX];
char buf[NBUFSIZ];

user_t user;

/*
 *  save uid to uids[]
 */
void
saveuid() {
    char uid[5];
    int cnt = 0;
    int i, k;
    for (i = 0; cnt < 2; i++)
        if (buf[i] == ':')
            cnt++;
    k = i;
    while (buf[i] != ':') {
        uid[i - k] = buf[i];
        i++;
    }
    uid[i - k] = 0;
    uids[(int)strtol(uid, NULL, 10)] = 1;
}

void
savegid() {
    char gid[5];
    int i;
    for (i = 0; buf[i] != ':'; i++)
        gid[i] = buf[i];
    gid[i] = 0;
    gids[(int)strtol(gid, NULL, 10)] = 1;
}

/*
 *  Returns lowest free uid if exists
 */
uid_t
findfreeuid() {
    int r;
    int fd = open("/etc/passwd", O_RDONLY);
    if (fd < 0)
        return 1;
    do {
        r = getline(fd, buf, NBUFSIZ);
        saveuid();
    } while (r > 0);
    close(fd);
    for (int i = 1; i < UID_MAX; i++)
        if (!uids[i])
            return i;
    printf("No free uids\n");
    return -1; // out of uids
}

gid_t
findfreegid() {
    int r;
    int fd = open("/etc/group", O_RDONLY);
    if (fd < 0)
        return 1;
    do {
        r = getline(fd, buf, NBUFSIZ);
        savegid();
    } while (r > 0);
    close(fd);
    for (int i = 1; i < UID_MAX; i++)
        if (!gids[i])
            return i;
    printf("No free gids\n");
    return -1;
}

/*
 * set defaults for user
 */
void
userinit() {
    user.u_uid = findfreeuid();
    if (user.u_uid == -1)
        exit();
    strncpy(user.u_home, "/home/", 6);
    user.u_home[6] = 0;
    strncpy(user.u_home + 6, user.u_comment,
            strlen(user.u_comment) > (PATHLEN_MAX - 6) ? (PATHLEN_MAX - 6) : strlen(user.u_comment));
    user.u_primgrp = findfreegid();
    strncpy(user.u_shell, "/sh", 3);
    user.u_shell[3] = 0;
    user.u_password[0] = 0;
}

void
itoa(int i, char* string) {
    int power = 0, j = 0;
    int k = 0;
    j = i;
    for (power = 1; j >= 10; j /= 10)
        power *= 10;
    for (; power > 0; power /= 10) {
        string[k] = '0' + i / power;
        k++;
        i %= power;
    }
    string[k] = 0;
}

void
makearg(char* giduid, uid_t uid, gid_t gid) {
    itoa(gid, giduid);
    int i = 0;
    while (giduid[i])
        i++;
    giduid[i] = ':';
    i++;
    itoa(uid, giduid + i);
}

void
writepass(const char* pass) {
    int fd = open("/etc/shadow", O_WRONLY | O_CREAT | O_APPEND);
    if (fd < 0) {
        exit();
    }
    char salt[20] = {"qqqqqqqqqqqqqqqqqqq\0"};
    int i = 0;
    printf("Enter salt for hash\n");
    while(1) {
        salt[i] = getchar();
        if(salt[i] == '\n' || salt[i] == '\r')
            break;
        if(salt[i] <= 0)
            break;
        i++;
        if(i >= 19)
            break;
    }
    salt[i] = 0;
    char hash[20] = {0};
    pkcs5_pbkdf2((uint8_t *)pass, strlen(pass), (const uint8_t *)salt, 20, (uint8_t *)hash, 20, 1024);
    char b64hash[33];
    bintob64(b64hash, hash, strlen(hash));
    fprintf(fd, "%s:$0$:%s$%s:\n", user.u_comment, salt, b64hash);
    close(fd);
}

/*
 * write or update userinfo to /etc/passwd
 */
void
useradd() {
    int fd = open("/etc/passwd", O_WRONLY | O_CREAT | O_APPEND);
    if(fd < 0)
        exit();
    fprintf(fd, "%s:%s:%d:%d::%s:%s\n", user.u_comment, "x", user.u_uid,
            user.u_primgrp, user.u_home, user.u_shell);
    close(fd);
    writepass(user.u_password);
    int r;
    r = spawnl("/mkdir", "/mkdir", user.u_home, NULL);
    if (r < 0) {
        printf("Incorrect HOMEPATH\n");
        exit();
    }
    if (r >= 0)
        wait(r);
    char giduid[10];
    makearg(giduid, user.u_uid, user.u_primgrp);
    char uidarg[4];
    itoa(user.u_uid, uidarg);
    int s = 0;
    while (giduid[s] != ':') {
        s++;
    }
    giduid[s] = 0;
    r = spawnl("/groupmod", "/groupmod", giduid, giduid + s + 1, NULL);
    if (r >= 0)
        wait(r);
    giduid[s] = ':';
    r = spawnl("/chown", "/chown", giduid, user.u_home, NULL);
    if (r >= 0)
        wait(r);
    r = spawnl("/chmod", "/chmod", "0755", user.u_home, NULL);
    if (r >= 0)
        wait(r);
}

void
usage() {
    printf("usage:useradd [-u UID] [-g GID] [-b HOMEPATH] [-s SHELLPATH] [-p PASSWORD] LOGIN\n");
    exit();
}

/*
 * find chars from set in str. Returns first finded char or 0
 */
char
strpbrk(char* str, char* set) {
    for (int i = 0; str[i]; i++) {
        for (int j = 0; set[j]; j++) {
            if (str[i] == set[j])
                return str[i];
        }
    }
    return 0;
}

/*
 *  parse nonflag args to user_t user
 */
int
fillargs(int argc, char** argv) {
    for (int i = 0; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (strlen(argv[i]) != 2)
                usage();
            char res = strpbrk(argv[i], "bpgus");
            if (!res) continue;
            if (i + 1 == argc || argv[i + 1][0] == '-') return 1;
            if (res == 'u') {
                flag['u'] = 1;
                uid_t uid = (uid_t)strtol(argv[i + 1], NULL, 10);
                if (uid > 0 && uid < UID_MAX) {
                    user.u_uid = uid;
                } else {
                    printf("UID should be > 0 and < %d\n", UID_MAX);
                    exit();
                }
                if (isuserexist(uid)) {
                    printf("Uid is already in use\n");
                    exit();
                }
            } else
            if (res == 'p') {
                flag['p'] = 1;
                int len = strlen(argv[i + 1]) > PASSLEN_MAX ? PASSLEN_MAX : strlen(argv[i + 1]);
                strncpy(user.u_password, argv[i + 1], len);
                user.u_password[len] = 0;
            } else
            if (res == 's') {
                flag['s'] = 1;
                int len = strlen(argv[i + 1]) > PATHLEN_MAX ? PATHLEN_MAX : strlen(argv[i + 1]);
                strncpy(user.u_shell, argv[i + 1], len);
                user.u_shell[len] = 0;
            } else
            if (res == 'b') {
                flag['b'] = 1;
                int len = strlen(argv[i + 1]) > PATHLEN_MAX ? PATHLEN_MAX : strlen(argv[i + 1]);
                strncpy(user.u_home, argv[i + 1], len);
                user.u_home[len] = 0;
                if (!strcmp("/", user.u_home)) {
                    printf("Homepath could not be /\n");
                    exit();
                }
            } else
            if (res == 'g') {
                flag['g'] = 1;
                gid_t gid = (gid_t)strtol(argv[i + 1], NULL, 10);
                if (gid > 0 && gid < UID_MAX)
                    user.u_primgrp = gid;
                else {
                    printf("GID should be > 0 and < %d\n", UID_MAX);
                    exit();
                }
            }
            else
                usage();
        }
    }
    return 0;
}

void
fillname(int argc, char** argv) {
    if (argv[argc - 1][0] != '-' && !(strpbrk(argv[argc - 2], "bpgus") && strpbrk(argv[argc - 2], "-"))) {
        int len = strlen(argv[argc - 1]) > COMMENTLEN_MAX ? COMMENTLEN_MAX : strlen(argv[argc - 1]);
        strncpy(user.u_comment, argv[argc - 1], len);
        user.u_comment[len] = 0;
    } else
        usage();
}

void
namecheck(int argc, char** argv) {
    int r;
    if (argv[argc - 2][0] == '-')
        usage();
    int fd = open("/etc/passwd", O_RDONLY);
    do {
        r = getline(fd, buf, NBUFSIZ);
        int i;
        for (i = 0; buf[i] != ':'; i++)
            ;

        if (!strncmp(argv[argc - 1], buf, i) && strlen(argv[argc - 1]) == i) {
            printf("Username is already in user\n");
            exit();
        }
    } while (r > 0);
    close(fd);
}

int
countflags() {
    return flag['p'] + flag['s'] + flag['u'] + flag['g'] + flag['b'];
}

void
umain(int argc, char** argv) {
    int oldargc = argc;
    namecheck(argc, argv);
    fillname(argc, argv);
    userinit();
    if (fillargs(argc, argv))
        usage();
    if (argc == 1) {
        usage();
        return;
    }
    if ((oldargc - 2) != 2 * (oldargc - countflags() - 2))
        usage();
    useradd();
}
