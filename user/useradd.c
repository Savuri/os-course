#include <inc/lib.h>
#include <user/user.h>
#include <inc/string.h>

int flag[256];
int uids[UID_MAX];
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

    for (int i = 1; i < UID_MAX; i++)
        if (!uids[i])
            return i;
    printf("No free uids\n");
    return -1; // out of uids
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
    user.u_primgrp = user.u_uid;
    strncpy(user.u_shell, "/sh", 3);
    user.u_shell[3] = 0;
    user.u_password[0] = 0;
}

void
itoa(int i, char *string) {
	int power = 0, j = 0;
 
	j = i;
	for (power = 1; j>10; j /= 10)
		power *= 10;
 
	for (; power>0; power /= 10)
	{
		*string++ = '0' + i / power;
		i %= power;
	}
	*string = 0;
}

void
makearg(char* giduid, uid_t uid, gid_t gid) {
    itoa(gid, giduid);
    int i = 0;
    while(giduid[i])
        i++;
    giduid[i] = ':';
    i++;
    itoa(uid, giduid + i);
}

/*
 * write or update userinfo to /etc/passwd
 */
void
useradd() {
    /*for (int i = 0; i < 256; i++) {
        if (!flag[i]) continue;
        switch (i) {
        case 'D':
            // usermod();
            break;
        default:;
        }
    }*/
    int fd = open("/etc/passwd", O_WRONLY | O_CREAT | O_APPEND);
    fprintf(fd, "%s:%s:%d:%d::%s:%s\n", user.u_comment, user.u_password, user.u_uid,
            user.u_primgrp, user.u_home, user.u_shell);
    close(fd);
    int r;
    r = spawnl("/mkdir", "/mkdir", user.u_home, NULL);
    if (r >= 0)
        wait(r);
    char giduid[10];
    makearg(giduid, user.u_uid, user.u_primgrp);
    char uidarg[4];
    itoa(user.u_uid, uidarg);
    r = spawnl("/chown", "/chown", uidarg, user.u_home, NULL);
    if(r < 0)
        printf("err\n");
    if(r >= 0)
        wait(r);
    int s = 0;
    while(giduid[s] != ':'){
        s++;
    }
    giduid[s] = 0;
    r = spawnl("/groupmod", "/groupmod", giduid, giduid + s + 1, NULL);
    if(r < 0)
        printf("err\n");
    if(r >= 0)
        wait(r);
}

void
usage() {
    printf("usage:useradd [-g GROUP] [-b HOMEPATH] [-s SHELLPATH] [-p PASSWORD] LOGIN\n");
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
            char res = strpbrk(argv[i], "bpgus");
            if (!res) continue;
            if (i + 1 == argc || argv[i + 1][0] == '-') return 1;
            if (res == 'u') {
                uid_t uid = (uid_t)strtol(argv[i + 1], NULL, 10);
                if (uid > 0 && uid < UID_MAX)
                    user.u_uid = uid;
                else {
                    printf("UID should be > 1 and < %d", UID_MAX);
                    exit();
                }
            }
            if (res == 'p') {
                int len = strlen(argv[i + 1]) > PASSLEN_MAX ? PASSLEN_MAX : strlen(argv[i + 1]);
                strncpy(user.u_password, argv[i + 1], len);
                user.u_password[len] = 0;
            }
            if (res == 's') {
                int len = strlen(argv[i + 1]) > PATHLEN_MAX ? PATHLEN_MAX : strlen(argv[i + 1]);
                strncpy(user.u_shell, argv[i + 1], len);
                user.u_shell[len] = 0;
            }
            if (res == 'b') {
                int len = strlen(argv[i + 1]) > PATHLEN_MAX ? PATHLEN_MAX : strlen(argv[i + 1]);
                strncpy(user.u_home, argv[i + 1], len);
                user.u_home[len] = 0;
                if (!strcmp("/", user.u_home)) {
                    printf("Homepath could not be /\n");
                    exit();
                }
            }
            if (res == 'g') {
                gid_t gid = (gid_t)strtol(argv[i + 1], NULL, 10);
                if (gid > 0 && gid < UID_MAX)
                    user.u_primgrp = gid;
            }
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
umain(int argc, char** argv) {
    int i;
    struct Argstate args;
    fillname(argc, argv);
    userinit();
    if (fillargs(argc, argv))
        usage();
    argstart(&argc, argv, &args);
    if (argc == 1) {
        usage();
        return;
    }
    while ((i = argnext(&args)) >= 0) {
        switch (i) {
        case 'p':
        case 'g':
        case 'b':
        case 's':
        case 'u':
            flag[i]++;
            break;
        default:
            usage();
        }
    }
    useradd();
}
