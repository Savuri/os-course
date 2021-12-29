#include <inc/lib.h>
#include <user/user.h>
#include <inc/crypt.h>
#include <inc/base64.h>

#define ETC_SHADOW "/etc/shadow"
#define ETC_PASSWD "/etc/passwd"

typedef struct passwd_t {
    char *name;
    char *password;
    uid_t uid;
    gid_t gid;
    char *comment;
    char *homepath;
    char *shell;
} passwd_t;


typedef struct shadow_t {
    char *name;
    int type;
    char *salt;
    char *hash;
} shadow_t;

char nbuf[NBUFSIZ + 1];
char passbuf[NBUFSIZ + 1];
char hash[20 + 1];
char base64[32 + 1];

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
parse_shadow_entry(char *buf, shadow_t *shadow) {
    int i = 0;

    shadow->name = buf;
    for (i = 0; buf[i] != ':'; i++)
        if (buf[i] == '\0')
            return -1;
    buf[i] = '\0';
    i++;
    if (buf[i] != '$')
        return -1;
    i++;
    if (buf[i] == '\0')
        return -1;
    shadow->type = buf[i] - '0';
    i++;
    if (buf[i] != '$')
        return -1;
    i++;
    shadow->salt = buf + i;
    for (; buf[i] != '$'; i++)
        if (buf[i] == '\0')
            return -1;
    buf[i] = '\0';
    i++;
    shadow->hash = buf + i;
    for (; buf[i] != ':'; i++)
        if (buf[i] == '\0')
            return 0;
    buf[i] = '\0';
    return 0;
}

int
passcmp(char *input, char *src) {
    int res = 0;
    while (*src != '\0') {
        res |= *input ^ *src;
        input++;
        src++;
    }
    return res;
}

int
checkpassword(const char login[], const char password[], passwd_t *passwd) {
    shadow_t shadow;
    int f, res;

    f = open(ETC_SHADOW, O_RDONLY);
    if (f < 0) {
        printf("Can't open " ETC_SHADOW ": %i\n", f);
        return f;
    }
    for (;;) {
        res = getline(f, passbuf, NBUFSIZ);
        if (res == 0) {
            res = -3;
            break;
        }
        res = parse_shadow_entry(passbuf, &shadow);
        if (res < 0) {
            return -1;
        }
        if (!strcmp(login, shadow.name)) {
            res = pkcs5_pbkdf2((uint8_t *)password, strlen(password),
                               (uint8_t *)shadow.salt, strlen(shadow.salt),
                               (uint8_t *)hash, 20,
                               1024);
            if (res < 0)
                return res;
            bintob64(base64, hash, strlen(hash));
            res = passcmp(base64, shadow.hash) ? -4 : 1;
            break;
        }
    }
    memset(passbuf, 0, sizeof(passbuf));
    memset(base64, 0, sizeof(base64));
    memset(hash, 0, sizeof(hash));
    close(f);
    return res;
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
            res = checkpassword(login, password, passwd);
            // res = strcmp(password, passwd->password) ? -1 : 1;
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
            if (ch == '\n' || ch == '\r')
                break; /* OK */
            if (ch <= 0)
                exit(); /* EOF -> restart login */

            if ((ch == '\b' || ch == '\x7F')) {
                if (p != nbuf) {
                    cputchar('\b');
                    cputchar(' ');
                    cputchar('\b');
                    p--;
                }
            } else if (p < nbuf + COMMENTLEN_MAX) {
                *p = ch;
                p++;
                cputchar(ch);
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
        if (ch == '\n' || ch == '\r')
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
