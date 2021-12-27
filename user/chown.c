#include <inc/lib.h>
#include <user/user.h>
#include <inc/string.h>

void
usage() {
    printf("usage:chown UID FILEPATH\n");
    exit();
}

/*
 * -1 no finds
 * -2 more than one :
 *  pos :
 */
int
finddelim(char * arg) {
    long int i = 0;
    long int res;
    while((arg[i] != ':') && (i < strlen(arg)))
        i++;
    if(arg[i] == ':'){
        res = i;
        while(i < strlen(arg)){
            i++;
            if(arg[i] == ':')
                res = -2;
        }
        return res;
    }
    return -1;
}

void
umain(int argc, char** argv) {
    if(argc != 3){
        usage();
        return;
    }
    int delim = finddelim(argv[1]);
    if(delim < 0)
        usage();
    argv[delim] = 0;
    //path check?
    if(chown(argv[2], (uid_t)(strtol(argv[1], NULL, 10)))){
        printf("Bad chown\n");
        exit();
    }
    if(chgrp(argv[2], (gid_t)(strtol(argv[1]+delim+1, NULL, 10))))
        printf("Bad chgrp\n");
}