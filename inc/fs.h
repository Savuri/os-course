/* See COPYRIGHT for copyright information. */

#ifndef JOS_INC_FS_H
#define JOS_INC_FS_H

#include <inc/types.h>
#include <inc/mmu.h>

typedef int32_t envid_t;
typedef uint32_t blockno_t;

/* File nodes (both in-memory and on-disk) */

/* Bytes per file system block - same as page size */
#define BLKSIZE    PAGE_SIZE
#define BLKBITSIZE (BLKSIZE * 8)

/* Maximum size of a filename (a single path component), including null
 * Must be a multiple of 4 */
#define MAXNAMELEN 128

/* Maximum size of a complete pathname, including null */
#define MAXPATHLEN 1024

/* Number of block pointers in a File descriptor */
#define NDIRECT 10
/* Number of direct block pointers in an indirect block */
#define NINDIRECT (BLKSIZE / 4)

#define MAXFILESIZE ((NDIRECT + NINDIRECT) * BLKSIZE)

#define SETBIT(v, n) ((v)[(n / 32)] |= 1U << ((n) % 32))
#define CLRBIT(v, n) ((v)[(n / 32)] &= ~(1U << ((n) % 32)))
#define TSTBIT(v, n) ((v)[(n / 32)] & (1U << ((n) % 32)))

#define S_ISUID 04000                                              /* Set user ID on execution.  */
#define S_ISGID 02000                                              /* Set group ID on execution.  */
#define S_ISVTX 01000 /* Save swapped text after use (sticky).  */ // TODO: Did we need it?

#define S_IRUSR 0400                          /* Read by owner.  */
#define S_IWUSR 0200                          /* Write by owner.  */
#define S_IXUSR 0100                          /* Execute by owner.  */
#define S_IRWXU (S_IRUSR | S_IWUSR | S_IXUSR) /* Read, write, and execute by owner.  */

#define S_IRGRP (S_IRUSR >> 3) /* Read by group.  */
#define S_IWGRP (S_IWUSR >> 3) /* Write by group.  */
#define S_IXGRP (S_IXUSR >> 3) /* Execute by group.  */
#define S_IRWXG (S_IRWXU >> 3) /* Read, write, and execute by group.  */

#define S_IROTH (S_IRGRP >> 3) /* Read by others.  */
#define S_IWOTH (S_IWGRP >> 3) /* Write by others.  */
#define S_IXOTH (S_IXGRP >> 3) /* Execute by others.  */
#define S_IRWXO (S_IRWXG >> 3) /* Read, write, and execute by others.  */

typedef uint16_t permission_t; // TODO: rename to understandable mode_t

struct Fcred {
    uid_t fc_uid;
    gid_t fc_gid;

    permission_t fc_permission; // TODO: rename to understandable fc_mode
};

struct File {
    char f_name[MAXNAMELEN]; /* filename */
    off_t f_size;            /* file size in bytes */
    uint32_t f_type;         /* file type */

    /* Block pointers. */
    /* A block is allocated iff its value is != 0. */
    blockno_t f_direct[NDIRECT]; /* direct blocks */
    blockno_t f_indirect;        /* indirect block */

    struct File *parent; /* for files - their dir, for dir - parent dir. */

    struct Fcred f_cred;
    /* Pad out to 256 bytes; must do arithmetic in case we're compiling
     * fsformat on a 64-bit machine. */
    uint8_t f_pad[256 - MAXNAMELEN - 8 - 4 * NDIRECT - 4 - sizeof(struct Fcred) - sizeof(struct File *)];
} __attribute__((packed)); /* required only on some 64-bit machines */

/* An inode block contains exactly BLKFILES 'struct File's */
#define BLKFILES (BLKSIZE / sizeof(struct File))

/* File types */
#define FTYPE_REG 0 /* Regular file */
#define FTYPE_DIR 1 /* Directory */

/* File system super-block (both in-memory and on-disk) */

#define FS_MAGIC 0x4A0530AE /* related vaguely to 'J\0S!' */

struct Super {
    uint32_t s_magic;    /* Magic number: FS_MAGIC */
    blockno_t s_nblocks; /* Total number of blocks on disk */
    struct File s_root;  /* Root directory node */
};

/* Definitions for requests from clients to file system */
enum {
    FSREQ_OPEN = 1,
    FSREQ_SET_SIZE,
    /* Read returns a Fsret_read on the request page */
    FSREQ_READ,
    FSREQ_WRITE,
    /* Stat returns a Fsret_stat on the request page */
    FSREQ_STAT,
    FSREQ_FLUSH,
    FSREQ_REMOVE,
    FSREQ_SYNC,
    FSREQ_CHMOD,
    FSREQ_CHOWN,
    FSREQ_CHGRP,
    FSREQ_ACCESSDIR,
    FSREQ_SET_CHILD_CRED,
};

union Fsipc {
    struct Fsreq_open {
        char req_path[MAXPATHLEN];
        int req_omode;
    } open;
    struct Fsreq_set_size {
        int req_fileid;
        off_t req_size;
    } set_size;
    struct Fsreq_read {
        int req_fileid;
        size_t req_n;
    } read;
    struct Fsret_read {
        char ret_buf[PAGE_SIZE];
    } readRet;
    struct Fsreq_write {
        int req_fileid;
        size_t req_n;
        char req_buf[PAGE_SIZE - (2 * sizeof(size_t))];
    } write;
    struct Fsreq_stat {
        int req_fileid;
    } stat;
    struct Fsret_stat {
        char ret_name[MAXNAMELEN];
        off_t ret_size;
        int ret_isdir;
        struct Fcred ret_fcred;
    } statRet;
    struct Fsreq_flush {
        int req_fileid;
    } flush;
    struct Fsreq_remove {
        char req_path[MAXPATHLEN];
    } remove;
    struct Fsreq_chmod {
        char req_path[MAXPATHLEN];
        permission_t req_perm;
    } chmod;
    struct Fsreq_chown {
        char req_path[MAXPATHLEN];
        uid_t req_uid;
    } chown;
    struct Fsreq_chgrp {
        char req_path[MAXPATHLEN];
        gid_t req_gid;
    } chgrp;
    struct Fsreq_accessdir {
        char req_path[MAXPATHLEN];
    } accessdir;
    struct Fsreq_set_child_cred {
        int req_fileid;
        envid_t req_envid;
    } set_child_cred;
    /* Ensure Fsipc is one page */
    char _pad[PAGE_SIZE];
};

#endif /* !JOS_INC_FS_H */
