#include <inc/lib.h>


#define NBUFSIZ 64
#define ETC_SHADOW "etc/shadow"
#define ETC_PASSWD "etc/passwd"

/*
void
readfile()
{
    int f, res, ch, i;
    char pw_name[NBUFSIZ];

    f = open(ETC_PASSWD, O_RDONLY);
    for (;;) {
        i = 0;
        for (;;) {
            ch = fgetc(f);
            if (ch == EOF)
                break;
            pw_name[i] = ch;
            i++
        }
        pw_name[i] = '\0';
    }

}
*/

int
authcheck(char login[], char password[])
{
    return 1000;
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
            if (p < nbuf + (NBUFSIZ - 1)) {
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
        if (p < nbuf + (NBUFSIZ - 1)) {
            *p = ch;
            p++;
        }
    }
    *p = '\0'; /* Success */
    /* Restore terminal state */
}


void
umain(int argc, char *argv[]) {
    uid_t uid;
    int r;


    if (sys_seteuid(0) < 0) {
        //sleep(2);
        return ;
    }

    for (;;) { /* TODO count failed attempts */
        char username[NBUFSIZ] = {0};
        char password[NBUFSIZ] = {0};

        getloginname(username);
        getpassword(password);
        uid = authcheck(username, password);
        if (uid >= 0)
            break;
        //sleep(5); /* TODO carefully calculate sleep time*/
        printf("\nLogin failed.\n");
    }

    sys_setuid(uid);
    sys_setgid(uid);
    r = spawnl("/sh", "sh", (char *)0);
    if (r < 0) {
        printf("login: spawn shell: %i\n", r);
        return ;
    }
    wait(r);
}
