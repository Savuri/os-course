/*
 * JOS file system format
 */

/* We don't actually want to define off_t! */
#define off_t xxx_off_t
#define bool xxx_bool
#include <assert.h>
#include <errno.h>
#include <mqueue.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <dirent.h>

#undef off_t
#undef bool

/* Prevent inc/types.h, included from inc/fs.h,
 * from attempting to redefine types defined in the host's inttypes.h. */
#define JOS_INC_TYPES_H
/* Typedef the types that inc/mmu.h needs. */
typedef uint32_t physaddr_t;
typedef uint32_t off_t;
typedef int bool;

// TODO: make this beautiful
#undef S_ISUID
#undef S_ISGID
#undef S_ISVTX // TODO: Did we need it?
#undef S_IRUSR
#undef S_IWUSR
#undef S_IXUSR
#undef S_IRWXU
#undef S_IRGRP
#undef S_IWGRP
#undef S_IXGRP
#undef S_IRWXG
#undef S_IROTH
#undef S_IWOTH
#undef S_IXOTH
#undef S_IRWXO

#include <inc/mmu.h>
#include <inc/fs.h>
#include <sys/stat.h>

#define ROUNDUP(n, v) ((n == 0) ? (0) : ((n)-1 + (v) - ((n)-1) % (v)))
#define MAX_DIR_ENTS  128
#define DISKMAP       0x10000000

struct Dir {
    struct File *f;
    struct File *ents;
    int n;
};

uint32_t nblocks;
char *diskmap, *diskpos;
struct Super *super;
uint32_t *bitmap;

void
panic(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    abort();
}

void
readn(int f, void *out, size_t n) {
    size_t p = 0;
    while (p < n) {
        int m = read(f, out + p, n - p);
        if (m < 0)
            panic("read: %s", strerror(errno));
        if (m == 0)
            panic("read: Unexpected EOF");
        p += m;
    }
}

uint32_t
blockof(void *pos) {
    return ((char *)pos - diskmap) / BLKSIZE;
}

void *
alloc(uint32_t bytes) {
    void *start = diskpos;
    diskpos += ROUNDUP(bytes, BLKSIZE);
    if (blockof(diskpos) >= nblocks)
        panic("out of disk blocks");
    return start;
}

void
opendisk(const char *name) {
    int diskfd, nbitblocks;

    if ((diskfd = open(name, O_RDWR | O_CREAT, 0666)) < 0)
        panic("open %s: %s", name, strerror(errno));

    if (ftruncate(diskfd, 0) < 0 || ftruncate(diskfd, nblocks * BLKSIZE) < 0)
        panic("truncate %s: %s", name, strerror(errno));

    if ((diskmap = mmap(NULL, nblocks * BLKSIZE, PROT_READ | PROT_WRITE,
                        MAP_SHARED, diskfd, 0)) == MAP_FAILED)
        panic("mmap %s: %s", name, strerror(errno));

    close(diskfd);

    diskpos = diskmap;
    alloc(BLKSIZE);
    super = alloc(BLKSIZE);
    super->s_magic = FS_MAGIC;
    super->s_nblocks = nblocks;
    super->s_root.f_type = FTYPE_DIR;
    strcpy(super->s_root.f_name, "/");

    nbitblocks = (nblocks + BLKBITSIZE - 1) / BLKBITSIZE;
    bitmap = alloc(nbitblocks * BLKSIZE);
    memset(bitmap, 0xFF, nbitblocks * BLKSIZE);
}

void
finishdisk(void) {
    int i;

    for (i = 0; i < blockof(diskpos); ++i)
        bitmap[i / 32] &= ~(1U << (i % 32));

    if (msync(diskmap, nblocks * BLKSIZE, MS_SYNC) < 0)
        panic("msync: %s", strerror(errno));
}

void
finishfile(struct File *f, uint32_t start, uint32_t len) {
    int i;
    f->f_size = len;
    len = ROUNDUP(len, BLKSIZE);
    for (i = 0; i < len / BLKSIZE && i < NDIRECT; ++i)
        f->f_direct[i] = start + i;
    if (i == NDIRECT) {
        uint32_t *ind = alloc(BLKSIZE);
        f->f_indirect = blockof(ind);
        for (; i < len / BLKSIZE; ++i)
            ind[i - NDIRECT] = start + i;
    }
}

void
startdir(struct File *f, struct Dir *dout) {
    dout->f = f;
    dout->ents = malloc(MAX_DIR_ENTS * sizeof *dout->ents);
    dout->n = 0;
}

struct File *
diradd(struct Dir *d, uint32_t type, const char *name) {
    struct File *out = &d->ents[d->n++];
    if (d->n > MAX_DIR_ENTS)
        panic("too many directory entries");
    strcpy(out->f_name, name);
    out->f_type = type;
    return out;
}

void
finishdir(struct Dir *d) {
    int size = d->n * sizeof(struct File);
    struct File *start = alloc(size);
    memmove(start, d->ents, size);
    finishfile(d->f, blockof(start), ROUNDUP(size, BLKSIZE));
    free(d->ents);  // COMMENT FOR use dump_dir, dump_file [debug]
    d->ents = NULL; // COMMENT FOR use dump_dir, dump_file [debug]
}

void
writefile(struct Dir *dir, const char *name) {
    int fd;
    struct File *f;
    struct stat st;
    const char *last;
    char *start;

    if ((fd = open(name, O_RDONLY)) < 0)
        panic("open %s: %s", name, strerror(errno));
    if (fstat(fd, &st) < 0)
        panic("stat %s: %s", name, strerror(errno));
    if (!S_ISREG(st.st_mode))
        panic("%s is not a regular file", name);
    if (st.st_size >= MAXFILESIZE)
        panic("%s too large", name);

    last = strrchr(name, '/');
    if (last)
        last++;
    else
        last = name;

    f = diradd(dir, FTYPE_REG, last);
    f->f_cred.fc_uid = 0;
    f->f_cred.fc_gid = 0;

    if (*name == 'o') {
        f->f_cred.fc_permission = S_IRWXU | S_IRWXG | S_IRWXO;
    } else if (strcmp(name, "fs/load/read-only")) {
        f->f_cred.fc_permission = S_IRUSR | S_IRGRP | S_IROTH;
    } else if (strcmp(name, "fs/load/write-only")) {
        f->f_cred.fc_permission = S_IWUSR | S_IWGRP | S_IWOTH;
    } else {
        f->f_cred.fc_permission = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP /*| S_IROTH     */;
    }

    start = alloc(st.st_size);
    readn(fd, start, st.st_size);
    finishfile(f, blockof(start), st.st_size);
    close(fd);
}

void
usage(void) {
    fprintf(stderr, "Usage: fsformat fs.img NBLOCKS files...\n");
    exit(2);
}

/*
 * debug function
 */
void
dump_file(struct File *file) {
    fprintf(stderr, "Name:[%s]\n", file->f_name);
    fprintf(stderr, "Type:[%d]\n", file->f_type);
    fprintf(stderr, "direct blocks: ");

    for (int i = 0; i < NDIRECT; ++i) {
        fprintf(stderr, "[%d] ", file->f_direct[i]);
    }

    fprintf(stderr, "\n");
    fprintf(stderr, "indirect:[%u] \n", file->f_indirect);
    fprintf(stderr, "uid:[%u] \n", file->f_cred.fc_uid);
    fprintf(stderr, "gid:[%u] \n", file->f_cred.fc_gid);
    fprintf(stderr, "acc:[%u] \n", file->f_cred.fc_permission);
}

/*
 * debug function
 */
void
dump_dir(struct Dir *dir) {
    dump_file(dir->f);

    for (int i = 0; i < dir->n; i++) {
        dump_file(&dir->ents[i]);
    }

    fprintf(stderr, "\n");
}

/*
 * write dir (including files and subdirs) and it's content
 */
void
writedir(struct Dir *root, const char *full_name) {
    const char *expected_dir_location = "fs/";
    assert(strncmp(full_name, expected_dir_location, 3) == 0);

    struct Dir ldir; // loader interpretation of dir

    const char *name = strrchr(full_name, '/');
    if (name) {
        name++;
    } else {
        name = full_name;
    }

    struct File *jdir = diradd(root, FTYPE_DIR, name); // Dir struct in JOS

    jdir->f_cred.fc_permission = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    jdir->f_cred.fc_uid = 0;
    jdir->f_cred.fc_gid = 0;

    startdir(jdir, &ldir);
    DIR *dir; // Linux dir
    struct dirent *ent;

    if ((dir = opendir(full_name)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
                continue;
            }

            struct stat st;

            size_t l1 = strlen(full_name), l2 = strlen(ent->d_name);

            char *full_entry_name = malloc(l1 + l2 + 2);
            strncpy(full_entry_name, full_name, l1);
            full_entry_name[l1] = '/';
            strncpy(full_entry_name + l1 + 1, ent->d_name, l2);
            full_entry_name[l1 + l2 + 1] = '\0';


            if (stat(full_entry_name, &st) < 0) {
                panic("stat failure: %s\n", full_entry_name);
            }

            if (S_ISREG(st.st_mode)) {
                writefile(&ldir, full_entry_name);
            } else if (S_ISDIR(st.st_mode)) {
                writedir(&ldir, full_entry_name);
            } else {
                panic("Unsupported file type: %s\n", ent->d_name);
            }

            free(full_entry_name);
        }

        closedir(dir);
    } else {
        panic("Dir does not open [%s] %s\n", full_name, strerror(errno));
    }
    finishdir(&ldir);
}

void *
disk_address(blockno_t blockno, off_t offset) {
    return (void *)(DISKMAP + blockno * BLKSIZE + offset);
}

void *
map_address(blockno_t blockno) {
    return (void *)(diskmap + blockno * BLKSIZE);
}


void
init_parent_field(struct File *dir, struct File *parent_address_in_jos) {
    if (strcmp(dir->f_name, "/") == 0) {
        dir->parent = (struct File *)((char *)DISKMAP + BLKSIZE + sizeof(struct Super) - sizeof(struct File));
    }

    for (int i = 0; i < NDIRECT; ++i) {
        if (dir->f_direct[i] == 0) {
            return;
        }

        struct File *f = map_address(dir->f_direct[i]);

        for (int j = 0; j < BLKSIZE / sizeof(struct File); ++j) {
            if (f[j].f_name[0] != '\0') {
                f[j].parent = parent_address_in_jos;

                if (f[j].f_type == FTYPE_DIR) {
                    init_parent_field(&f[j], disk_address(dir->f_direct[i], j * sizeof(struct File)));
                }
            }
        }
    }

    if (dir->f_indirect == 0) {
        return;
    }

    blockno_t *block_arr = map_address(dir->f_indirect);

    for (int i = 0; i < BLKSIZE / sizeof(blockno_t); ++i) {
        struct File *f = map_address(block_arr[i]);

        for (int j = 0; j < BLKSIZE / sizeof(struct File); ++j) {
            if (f[j].f_name[0] != '\0') {
                f[j].parent = parent_address_in_jos;

                if (f[j].f_type == FTYPE_DIR) {
                    init_parent_field(&f[j], disk_address(dir->f_direct[i], j * sizeof(struct File)));
                }
            }
        }
    }
}

int
main(int argc, char **argv) {
    int i;
    char *s;
    struct Dir root;

    assert(BLKSIZE % sizeof(struct File) == 0);

    if (argc < 3)
        usage();

    nblocks = strtol(argv[2], &s, 0);
    if (*s || s == argv[2] || nblocks < 2 || nblocks > 10240)
        usage();

    opendisk(argv[1]);

    startdir(&super->s_root, &root);
    for (i = 3; i < argc; i++) {
        struct stat st;

        if (stat(argv[i], &st) < 0) {
            panic("stat error %s\n", argv[i]);
        }

        if (S_ISDIR(st.st_mode)) {
            writedir(&root, argv[i]);
        } else if (S_ISREG(st.st_mode)) {
            writefile(&root, argv[i]);
        } else {
            panic("Unsupported file type: %s\n", argv[i]);
        }
    }

    root.f->f_cred.fc_permission = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    root.f->f_cred.fc_uid = 0;
    root.f->f_cred.fc_gid = 0;

    finishdir(&root);
    init_parent_field(root.f, (struct File *)(DISKMAP + BLKSIZE + sizeof(struct Super) - sizeof(struct File)));
    finishdisk();

    return 0;
}
