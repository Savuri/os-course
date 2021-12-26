#include <inc/lib.h>

#define NAMELEN    32
#define PASSLEN    64
#define NBUFSIZ    1024
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
getline(int f) {
    int i = 0, res;
    for (;;) {
        res = read(f, nbuf + i, sizeof(char));
        if (res <= 0)
            return -1;
        if (nbuf[i] == '\n')
            break;
        if (i >= NBUFSIZ)
            return -1;
        i++;
    }
    nbuf[i] = '\0';
    return 0;
}

int
parseline(passwd_t *passwd) {
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

    passwd->name = nbuf;
    for (int i = 0; nbuf[i] != '\0'; i++) {
        if (nbuf[i] == ':') { /* word ended */
            nbuf[i] = '\0';
            if (state == S_UID)
                passwd->uid = strtol(number, NULL, 10);
            if (state == S_GID)
                passwd->gid = strtol(number, NULL, 10);
            state++;
            switch (state) {
            case S_PASSWORD:
                passwd->password = nbuf + i + 1;
                break;
            case S_UID:
                number = nbuf + i + 1;
                break;
            case S_GID:
                number = nbuf + i + 1;
                break;
            case S_COMMENT:
                passwd->comment = nbuf + i + 1;
                break;
            case S_HOMEPATH:
                passwd->homepath = nbuf + i + 1;
                break;
            case S_SHELL:
                passwd->shell = nbuf + i + 1;
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
        res = getline(f);
        if (res < 0)
            break;
        res = parseline(passwd);
        if (res < 0)
            break;
        if (!strcmp(login, passwd->name)) {
            res = strcmp(password, passwd->password) ? -1 : 0;
            break;
        }
    }
    close(f);
    return res;
}

void
getloginname(char *nbuf) {
    static char *p;
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
            if (p < nbuf + NAMELEN) {
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
        if (p < nbuf + PASSLEN) {
            *p = ch;
            p++;
        }
    }
    *p = '\0'; /* Success */
    /* Restore terminal state */
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
        char username[NAMELEN + 1] = {0};
        char password[PASSLEN + 1] = {0};

        getloginname(username);
        getpassword(password);
        res = authcheck(username, password, &passwd);
        if (res >= 0)
            break;
        // sleep(5); /* TODO carefully calculate sleep time*/
        printf("\nLogin failed.\n");
    }
    printf("\n");

    sys_setgid(passwd.gid);
    /* change uid last, so we have privileges to set up everything */
    sys_setuid(passwd.uid);
    res = spawnl(passwd.shell, passwd.shell, (char *)0);
    if (res < 0) {
        printf("login: spawn shell: %i\n", res);
        return;
    }
    wait(res);
    return;
}
