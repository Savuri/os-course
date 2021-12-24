#include <inc/lib.h>
#include <user/user.h>
#include <inc/string.h>

int flag[256];

user_t user;

/*
 * set defaults for user
 */
void
userinit() {
    user.u_uid = 1;
    strncpy(user.u_home, "/", 1);
    user.u_home[1] = 0;
    strncpy(user.u_shell, "/sh", 3);
    user.u_shell[3] = 0;
    user.u_password[0] = 0;
    //user.u_primgrp[] = 0; gid = uid
}

/*
 * write or update userinfo to /etc/passwd 
 */
void
useradd() {
    for(int i = 0; i < 256; i++){
        if(!flag[i]) continue;
        switch(i){
            case 'D':
                //usermod();
                break;
            default:
                ;
        }
    }
    int fd = open("/etc/passwd", O_RDWR); //O_APPEND?
    fprintf(fd, "%s:%s:%d:%s:%s:%s", user.u_comment, user.u_password, user.u_uid,
        user.u_primgrp, user.u_home, user.u_shell);
}

void
usage() {
    printf("usage:useradd [-D] [-g GROUP] [-b HOMEPATH] [-s SHELLPATH] [-p PASSWORD] [LOGIN]\n");
    exit();
}

/*
 * find chars from set in str. Returns first finded char or 0
 */
char
strpbrk(char* str, char* set) {
    for(int i = 0; str[i]; i++){
        for(int j = 0; set[j]; j++){
            if(str[i] == set[j])
                return str[i];
        }
    }
    return 0;
}

int
fillargs(int argc, char ** argv) {
    for(int i = 0; i < argc; i++){
        if(argv[i][0] == '-'){
            char res = strpbrk(argv[i], "bpgus");
            if (!res) continue;
            if (i+1 == argc || argv[i+1][0] == '-') return 1;
            if (res == 'u'){
                user.u_uid = (uid_t)strtol(argv[i+1], NULL, 10);
            }
            if (res == 'p'){
                int len = strlen(argv[i+1]) > PASSLEN_MAX? PASSLEN_MAX : strlen(argv[i+1]);
                strncpy(user.u_password, argv[i+1], len);
                user.u_password[len] = 0;
            }
            if (res == 's'){
                int len = strlen(argv[i+1]) > PATHLEN_MAX? PATHLEN_MAX : strlen(argv[i+1]);
                strncpy(user.u_shell, argv[i+1], len);
                user.u_shell[len] = 0;
            }
            if (res == 'b'){
                int len = strlen(argv[i+1]) > PATHLEN_MAX? PATHLEN_MAX : strlen(argv[i+1]);
                strncpy(user.u_home, argv[i+1], len);
                user.u_home[len] = 0;
            }
            if (res == 'g'){
                int len = strlen(argv[i+1]) > GROUPLEN_MAX? GROUPLEN_MAX : strlen(argv[i+1]);
                strncpy(user.u_primgrp, argv[i+1], len);
                user.u_primgrp[len] = 0;
            }
        }
    }
    if(argv[argc-1][0] != '-' && !(strpbrk(argv[argc-2], "bpgus") && strpbrk(argv[argc-2], "-"))){
        int len = strlen(argv[argc-1]) > COMMENTLEN_MAX? COMMENTLEN_MAX : strlen(argv[argc-1]);
        strncpy(user.u_comment, argv[argc-1], len);
        user.u_comment[len] = 0;
    }
    return 0;
}

void
umain(int argc, char **argv) {
    int i;
    struct Argstate args;
    userinit();
    if(fillargs(argc, argv))
        usage();
    argstart(&argc, argv, &args);
    if(argc == 1){
        usage();
        return;
    }
    while ((i = argnext(&args)) >= 0){
        switch (i) {
        case 'p':
        case 'D':
        case 'g':
        case 'm':
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