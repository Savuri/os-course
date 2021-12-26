#include <inc/fs.h>
#include <inc/lib.h>

#define SECTSIZE 512                  /* bytes per disk sector */
#define BLKSECTS (BLKSIZE / SECTSIZE) /* sectors per block */

/* Disk block n, when in memory, is mapped into the file system
 * server's address space at DISKMAP + (n*BLKSIZE). */
#define DISKMAP 0x10000000

/* Maximum disk size we can handle (3GB) */
#define DISKSIZE 0xC0000000

extern struct Super *super; /* superblock */
extern uint32_t *bitmap;    /* bitmap blocks mapped in memory */

/* ide.c */
bool ide_probe_disk1(void);
void ide_set_disk(int diskno);
void ide_set_partition(uint32_t first_sect, uint32_t nsect);
int ide_read(uint32_t secno, void *dst, size_t nsecs);
int ide_write(uint32_t secno, const void *src, size_t nsecs);

/* bc.c */
void *diskaddr(uint32_t blockno);
void flush_block(void *addr);
void bc_init(void);

/* fs.c */
void fs_init(void);
int file_get_block(struct File *f, uint32_t file_blockno, char **pblk);
int file_create(const char *path, struct File **f, int type, const struct Ucred *ucred);
int file_block_walk(struct File *f, uint32_t filebno, uint32_t **ppdiskbno, bool alloc);
int file_open(const char *path, struct File **f, const struct Ucred *ucred, int mode);
ssize_t file_read(struct File *f, void *buf, size_t count, off_t offset);
ssize_t file_write(struct File *f, const void *buf, size_t count, off_t offset);
int file_set_size(struct File *f, off_t newsize);
void file_flush(struct File *f);
int file_remove(const char *path, const struct Ucred *ucred);
int file_chmod(const char *path, permission_t perm, const struct Ucred *ucred);
int file_chown(const char *path, uid_t uid, const struct Ucred *ucred);
int file_chgrp(const char *path, gid_t gid, const struct Ucred *ucred);
int accessdir(const char *path, const struct Ucred *ucred);
void fs_sync(void);
int groupmember(gid_t gid, const struct Ucred *cred);
int access(uint32_t type, struct Fcred fcred, int acc_mode, const struct Ucred *cred);
enum mode {
    READ = 00400,
    WRITE = 0200,
    EXEC = 0100,
    SUID = 04000,
    SGID = 02000
};

/* int  map_block(uint32_t); */
bool block_is_free(uint32_t blockno);
blockno_t alloc_block(void);

/* test.c */
void fs_test(void);
