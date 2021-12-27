#include <inc/lib.h>
#include <user/user.h>
#include <inc/string.h>

char buf[NBUFSIZ];
int grnum = 0;

typedef struct group {
    char line[256];
} group_t;

group_t group[1024];  

void
usage() {
    printf("usage:groupdel GID\n");
    exit();
}

void
updategroups(char * gid) {
    int fd = open("/etc/group", O_WRONLY | O_TRUNC);
    for(int i = 0; i < grnum; i++) {
        int k = 0;
        while(buf[k] != ':')
            k++;
        if(strncmp(group[i].line, gid, k))
            fprintf(fd, "%s\n", group[i].line);
    }
}

void
groupdel(char * gid) {
    int fd = open("/etc/group", O_RDONLY);
    if(fd < 0)
        exit();
    while(getline(fd, buf, NBUFSIZ) > 0){
        int i = 0;
        while(buf[i] != '\n')
            i++;
        buf[i] = 0;
        strncpy(group[grnum].line, buf, i);
        grnum++;
    }
    close(fd);
    updategroups(gid);
}

void
umain(int argc, char** argv) {
    if (argc != 2) {
        usage();
        return;
    }
    int s = (int)strtol(argv[1], NULL, 10);
    if(s > 0 && s < 1024)
        groupdel(argv[1]);
    else
        printf("GID should be > 0 and < 1024\n");
}