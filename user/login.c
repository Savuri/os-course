#include <inc/lib.h>
#include <user/user.h>

#define ETC_SHADOW "etc/shadow"
#define ETC_PASSWD "etc/passwd"

typedef struct passwd_t {
    char *name;
    char *password;
    uid_t uid;
    gid_t gid;
    char *comment;
    char *homepath;
    char *shell;
} passwd_t;

char nbuf[NBUFSIZ + 1];

int
parseline(passwd_t *passwd, char *buf) {
    char *number = NULL;

    enum State {
        S_NAME,
        S_PASSWORD,
        S_UID,
        S_GID,
        S_COMMENT,
        S_HOMEPATH,
        S_SHELL,
        S_ERROR
    } state = S_NAME;

    passwd->name = buf;
    for (int i = 0; buf[i] != '\0'; i++) {
        if (buf[i] == ':') { /* word ended */
            buf[i] = '\0';
            if (state == S_UID)
                passwd->uid = strtol(number, NULL, 10);
            if (state == S_GID)
                passwd->gid = strtol(number, NULL, 10);
            state++;
            switch (state) {
            case S_PASSWORD:
                passwd->password = buf + i + 1;
                break;
            case S_UID:
                number = buf + i + 1;
                break;
            case S_GID:
                number = buf + i + 1;
                break;
            case S_COMMENT:
                passwd->comment = buf + i + 1;
                break;
            case S_HOMEPATH:
                passwd->homepath = buf + i + 1;
                break;
            case S_SHELL:
                passwd->shell = buf + i + 1;
                break;
            default:
                return -1;
            }
        }
    }
    if (state != S_SHELL)
        return -1;
    return 0;
}

int
authcheck(char login[], char password[], passwd_t *passwd) {
    int f, res;

    f = open(ETC_PASSWD, O_RDONLY);
    if (f < 0) {
        printf("Can't open " ETC_PASSWD ": %i\n", f);
        return f;
    }
    for (;;) {
        res = getline(f, nbuf, NBUFSIZ);
        if (res == 0) {
            res = -2;
            break;
        }
        res = parseline(passwd, nbuf);
        if (res < 0)
            break;
        if (!strcmp(login, passwd->name)) {
            res = strcmp(password, passwd->password) ? -1 : 1;
            break;
        }
    }
    close(f);
    return res;
}

void
getloginname(char *nbuf) {
    char *p;
    int ch;

    for (;;) {
        printf("login: ");
        p = nbuf;
        for (;;) {
            ch = getchar();
            if (ch == '\n')
                break; /* OK */
            if (ch <= 0)
                exit(); /* EOF -> restart login */
            if (p < nbuf + COMMENTLEN_MAX) {
                *p = ch;
                p++;
            }
        }
        if (p > nbuf) {
            if (nbuf[0] == '-') {
                printf("login names may not start with '-'.\n");
                p = nbuf;
            } else {
                *p = '\0'; /* Success */
                break;
            }
        }
    }
    printf("\n");
}

void
getpassword(char *nbuf) {
    char *p = nbuf;
    int ch;

    printf("password: ");
    /* TODO Find a way to disable printing entered text
     * Probably implement terminal control sequences
     */
    for (;;) {
        ch = getchar();
        if (ch == '\n')
            break; /* OK */
        if (ch <= 0)
            exit(); /* EOF -> restart login */
        if (p < nbuf + PASSLEN_MAX) {
            *p = ch;
            p++;
        }
    }
    *p = '\0'; /* Success */
    /* Restore terminal state */
    printf("\n");
}


void
umain(int argc, char *argv[]) {
    passwd_t passwd;
    int res;

    res = sys_seteuid(0);
    if (res < 0) {
        printf("seteuid: %i\n", res);
        return;
    }

    for (;;) { /* TODO count failed attempts */
        char username[COMMENTLEN_MAX + 1] = {0};
        char password[PASSLEN_MAX + 1] = {0};

        getloginname(username);
        getpassword(password);
        res = authcheck(username, password, &passwd);
        if (res > 0)
            break;
        // sleep(5); /* TODO carefully calculate sleep time*/
        printf("\nLogin failed.\n");
    }
    printf("\n");

    sys_setgid(passwd.gid);
    /* change uid last, so we have privileges to set up everything */
    sys_setuid(passwd.uid);
    res = chdir(passwd.homepath);
    if (res < 0) {
        printf("Can't change dir to %s: %i\n", passwd.homepath, res);
        return;
    }
    res = spawnl(passwd.shell, passwd.shell, (char *)0);
    if (res < 0) {
        printf("login: spawn shell: %i\n", res);
        return;
    }
    wait(res);
    return;
}
