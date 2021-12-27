#include <inc/lib.h>
#include <user/user.h>
#include <inc/string.h>

void
usage() {
    printf("usage:chown UID FILEPATH\n");
    exit();
}

void
umain(int argc, char** argv) {
    if(argc != 3){
        usage();
        return;
    }
    if(!isuserexist((uid_t)(strtol(argv[1], NULL, 10)))) {
        printf("Invalid UID\n");
        return;
    }
    //path check?
    if(chown(argv[2], (uid_t)(strtol(argv[1], NULL, 10))))
        printf("Bad chown\n");
}