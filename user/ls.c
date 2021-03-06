#include <inc/lib.h>

int flag[256];

void lsdir(const char *, const char *);
void ls1(const char *, bool, off_t, const char *, struct Fcred);

void
ls(const char *path, const char *prefix) {
    int r;
    struct Stat st;

    if ((r = stat(path, &st)) < 0) {
        printf("stat %s: %i", path, r);
        exit();
    }
    if (st.st_isdir && !flag['d'])
        lsdir(path, prefix);
    else
        ls1(0, st.st_isdir, st.st_size, path, st.st_fcred);
}

void
lsdir(const char *path, const char *prefix) {
    int fd, n;
    struct File f;

    if ((fd = open(path, O_RDONLY)) < 0) {
        printf("open %s: %i", path, fd);
    }
    while ((n = readn(fd, &f, sizeof f)) == sizeof f)
        if (f.f_name[0])
            ls1(prefix, f.f_type == FTYPE_DIR, f.f_size, f.f_name, f.f_cred);
    if (n > 0) {
        printf("short read in directory %s", path);
    }
    if (n < 0) {
        printf("error reading directory %s: %i", path, n);
    }
}

void
get_unix_perm(permission_t x, unsigned char *res) {
    const unsigned char s[10] = "rwxrwxrwx";

    for (int i = 0; i < 9; i++) {
        if (x & (1 << (8 - i))) {
            res[i] = s[i];
        } else {
            res[i] = '-';
        }
    }

    if (x & S_ISUID) {
        if (res[2] == 'x') {
            res[2] = 's';
        } else {
            res[2] = 'S';
        }
    }

    if (x & S_ISGID) {
        if (res[5] == 'x') {
            res[5] = 's';
        } else {
            res[5] = 'S';
        }
    }

    res[9] = '\0';
}

void
ls1(const char *prefix, bool isdir, off_t size, const char *name, struct Fcred fcred) {
    const char *sep;

    if (flag['l']) {
        unsigned char s[10];
        get_unix_perm(fcred.fc_permission, s);
        printf("%c%s %3d %3d %11d ", isdir ? 'd' : '-', s, fcred.fc_uid, fcred.fc_gid, size);
    }
    if (prefix) {
        if (prefix[0] && prefix[strlen(prefix) - 1] != '/')
            sep = "/";
        else
            sep = "";
        printf("%s%s", prefix, sep);
    }
    printf("%s", name);
    if (flag['F'] && isdir)
        printf("/");
    printf("\n");
}

void
usage(void) {
    printf("usage: ls [-dFl] [file...]\n");
    exit();
}

void
umain(int argc, char **argv) {
    int i;
    struct Argstate args;

    argstart(&argc, argv, &args);
    while ((i = argnext(&args)) >= 0)
        switch (i) {
        case 'd':
        case 'F':
        case 'l':
            flag[i]++;
            break;
        default:
            usage();
        }

    if (argc == 1) {
        char cur_dir[MAXPATHLEN];
        sys_getenvcurpath(cur_dir, 0);
        ls(cur_dir, "");
    } else {
        for (i = 1; i < argc; i++)
            ls(argv[i], argv[i]);
    }
}
