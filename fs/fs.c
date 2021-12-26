#include <inc/string.h>
#include <inc/partition.h>

#include "fs.h"

/* Superblock */
struct Super *super;
/* Bitmap blocks mapped in memory */
uint32_t *bitmap;

/****************************************************************
 *                         Super block
 ****************************************************************/

/* Validate the file system super-block. */
void
check_super(void) {
    if (super->s_magic != FS_MAGIC)
        panic("bad file system magic number %08x", super->s_magic);

    if (super->s_nblocks > DISKSIZE / BLKSIZE)
        panic("file system is too large");

    cprintf("superblock is good\n");
}

/****************************************************************
 *                         Free block bitmap
 ****************************************************************/

/* Check to see if the block bitmap indicates that block 'blockno' is free.
 * Return 1 if the block is free, 0 if not. */
bool
block_is_free(uint32_t blockno) {
    if (super == 0 || blockno >= super->s_nblocks) return 0;
    if (TSTBIT(bitmap, blockno)) return 1;
    return 0;
}

/* Mark a block free in the bitmap */
void
free_block(uint32_t blockno) {
    /* Blockno zero is the null pointer of block numbers. */
    if (blockno == 0) panic("attempt to free zero block");
    SETBIT(bitmap, blockno); // 1 bit === free block
}

/* Search the bitmap for a free block and allocate it.  When you
 * allocate a block, immediately flush the changed bitmap block
 * to disk.
 *
 * Return block number allocated on success,
 * 0 if we are out of blocks.
 *
 * Hint: use free_block as an example for manipulating the bitmap. */
blockno_t
alloc_block(void) {
    /* The bitmap consists of one or more blocks.  A single bitmap block
     * contains the in-use bits for BLKBITSIZE blocks.  There are
     * super->s_nblocks blocks in the disk altogether. */

    // LAB 10: Your code here

    // block_no = 0 reserved for errors
    // block_no = 1 reserved for superblock

    // setted up i bit in bitmap === i block is free
    for (blockno_t block_no = 2; block_no < super->s_nblocks; ++block_no) {
        if (block_is_free(block_no)) {
            CLRBIT(bitmap, block_no);
            flush_block(&bitmap[block_no / 32]);
            return block_no;
        }
    }

    cprintf("alloc_block: Out of blocks\n");
    return 0;
}

/* Validate the file system bitmap.
 *
 * Check that all reserved blocks -- 0, 1, and the bitmap blocks themselves --
 * are all marked as in-use. */
void
check_bitmap(void) {

    /* Make sure all bitmap blocks are marked in-use */
    for (uint32_t i = 0; i * BLKBITSIZE < super->s_nblocks; i++)
        assert(!block_is_free(2 + i));

    /* Make sure the reserved and root blocks are marked in-use. */

    assert(!block_is_free(1));
    assert(!block_is_free(0));

    cprintf("bitmap is good\n");
}

/****************************************************************
 *                    File system structures
 ****************************************************************/

/* Initialize the file system */
void
fs_init(void) {
    static_assert(sizeof(struct File) == 256, "Unsupported file size");

    /* Find a JOS disk.  Use the second IDE disk (number 1) if availabl */
    if (ide_probe_disk1())
        ide_set_disk(1);
    else
        ide_set_disk(0);
    bc_init();

    /* Set "super" to point to the super block. */
    super = diskaddr(1);
    check_super();

    /* Set "bitmap" to the beginning of the first bitmap block. */
    bitmap = diskaddr(2);

    check_bitmap();
}

/* Find the disk block number slot for the 'filebno'th block in file 'f'.
 * Set '*ppdiskbno' to point to that slot.
 * The slot will be one of the f->f_direct[] entries,
 * or an entry in the indirect block.
 * When 'alloc' is set, this function will allocate an indirect block
 * if necessary.
 *
 * Returns:
 *  0 on success (but note that *ppdiskbno might equal 0).
 *  -E_NOT_FOUND if the function needed to allocate an indirect block, but
 *      alloc was 0.
 *  -E_NO_DISK if there's no space on the disk for an indirect block.
 *  -E_INVAL if filebno is out of range (it's >= NDIRECT + NINDIRECT).
 *
 * Analogy: This is like pgdir_walk for files.
 * Hint: Don't forget to clear any block you allocate. */
int
file_block_walk(struct File *f, blockno_t filebno, blockno_t **ppdiskbno, bool alloc) {
    // LAB 10: Your code here
    if (filebno >= NDIRECT + NINDIRECT) return -E_INVAL;

    if (filebno < NDIRECT) {
        *ppdiskbno = f->f_direct + filebno;

        return 0;
    }

    if (!f->f_indirect) {
        if (!alloc) return -E_NOT_FOUND;
        blockno_t blockno = alloc_block();
        if (!blockno) return -E_NO_DISK;

        f->f_indirect = blockno;
        memset(diskaddr(f->f_indirect), 0, BLKSIZE);
    }

    *ppdiskbno = (blockno_t *)diskaddr(f->f_indirect) + filebno - NDIRECT;

    return 0;
}

/* Set *blk to the address in memory where the filebno'th
 * block of file 'f' would be mapped.
 *
 * Returns 0 on success, < 0 on error.  Errors are:
 *  -E_NO_DISK if a block needed to be allocated but the disk is full.
 *  -E_INVAL if filebno is out of range.
 *
 * Hint: Use file_block_walk and alloc_block. */
int
file_get_block(struct File *f, uint32_t filebno, char **blk) {
    // LAB 10: Your code here

    int res;
    blockno_t *pdiskbno;

    if ((res = file_block_walk(f, filebno, &pdiskbno, 1)) < 0) {
        return res;
    }

    blockno_t new_block;

    if (!*pdiskbno) {
        if ((new_block = alloc_block()) < 0) {
            return -E_NO_DISK;
        }

        *pdiskbno = new_block;
    }

    *blk = (char *)diskaddr(*pdiskbno);

    return 0;
}

/* Try to find a file named "name" in dir.  If so, set *file to it.
 *
 * Returns 0 and sets *file on success, < 0 on error.  Errors are:
 *  -E_NOT_FOUND if the file is not found */
static int
dir_lookup(struct File *dir, const char *name, struct File **file) {
    /* Search dir for name.
     * We maintain the invariant that the size of a directory-file
     * is always a multiple of the file system's block size. */
    assert((dir->f_size % BLKSIZE) == 0);
    blockno_t nblock = dir->f_size / BLKSIZE;
    for (blockno_t i = 0; i < nblock; i++) {
        char *blk;
        int res = file_get_block(dir, i, &blk);
        if (res < 0) return res;

        struct File *f = (struct File *)blk;
        for (blockno_t j = 0; j < BLKFILES; j++)
            if (strcmp(f[j].f_name, name) == 0) {
                *file = &f[j];
                return 0;
            }
    }
    return -E_NOT_FOUND;
}

/* Set *file to point at a free File structure in dir.  The caller is
 * responsible for filling in the File fields. */
static int
dir_alloc_file(struct File *dir, struct File **file) {
    char *blk;

    assert((dir->f_size % BLKSIZE) == 0);
    blockno_t nblock = dir->f_size / BLKSIZE;
    for (blockno_t i = 0; i < nblock; i++) {
        int res = file_get_block(dir, i, &blk);
        if (res < 0) return res;

        struct File *f = (struct File *)blk;
        for (blockno_t j = 0; j < BLKFILES; j++) {
            if (f[j].f_name[0] == '\0') {
                *file = &f[j];
                return 0;
            }
        }
    }
    dir->f_size += BLKSIZE;
    int res = file_get_block(dir, nblock, &blk);
    if (res < 0) return res;

    *file = (struct File *)blk;
    return 0;
}

/* Skip over slashes. */
static const char *
skip_slash(const char *p) {
    while (*p == '/')
        p++;
    return p;
}

/*
 * Check if gid is a member of the group set.
 * 0 - file group is not in user groups
 * 1 - file group is in user groups
 */
int
groupmember(gid_t gid, const struct Ucred *cred) {
    if (cred->cr_gid == gid)
        return 1;

    const gid_t *gp = cred->cr_groups;
    const gid_t *egp = &(cred->cr_groups[cred->cr_ngroups]);

    while (gp < egp) {
        if (*gp == gid)
            return 1;

        gp++;
    }

    return 0;
}

/* type      - FTYPE_REG or FTYPE_DIR
 * file_mode - file's permissions
 * req_uid       - file's creator req_uid
 * req_gid       - file's creator req_gid
 * acc_mode  - acc_type for operation
 * cred      - user's process credentials
 *
 * return 0 if accessible
 * otherwise return -E_ACCES
 *
 */
int
access(uint32_t type, struct Fcred fcred, int acc_mode, const struct Ucred *cred) {
    /* User id 0 always gets read/write access. */
    if (cred->cr_uid == 0) {
        /* For EXEC, at least one of the execute bits must be set. */
        if ((acc_mode & EXEC) && type != FTYPE_DIR && (fcred.fc_permission & (S_IXUSR | S_IXGRP | S_IXOTH)) == 0)
            return -E_ACCES;
        return 0;
    }

    permission_t mask = 0;

    /* Otherwise, check the owner. */
    if (cred->cr_uid == fcred.fc_uid) {
        if (acc_mode & EXEC)
            mask |= S_IXUSR;
        if (acc_mode & READ)
            mask |= S_IRUSR;
        if (acc_mode & WRITE)
            mask |= S_IWUSR;
        return (fcred.fc_permission & mask) == mask ? 0 : -E_ACCES;
    }

    /* Otherwise, check the groups. */
    if (groupmember(fcred.fc_gid, cred)) {
        if (acc_mode & EXEC)
            mask |= S_IXGRP;
        if (acc_mode & READ)
            mask |= S_IRGRP;
        if (acc_mode & WRITE)
            mask |= S_IWGRP;
        return (fcred.fc_permission & mask) == mask ? 0 : -E_ACCES;
    }

    /* Otherwise, check everyone else. */
    if (acc_mode & EXEC)
        mask |= S_IXOTH;
    if (acc_mode & READ)
        mask |= S_IROTH;
    if (acc_mode & WRITE)
        mask |= S_IWOTH;
    return (fcred.fc_permission & mask) == mask ? 0 : -E_ACCES;
}

/* Evaluate a path name, starting at the root.
 * On success, set *pf to the file we found
 * and set *pdir to the directory the file is in.
 * If we cannot find the file but find the directory
 * it should be in, set *pdir and copy the final path
 * element into lastelem. */
static int
walk_path(const char *path, struct File **pdir, struct File **pf, char *lastelem, const struct Ucred *ucred) {
    const char *p;
    char name[MAXNAMELEN];
    struct File *dir, *f;
    int r;

    // if (*path != '/')
    //     return -E_BAD_PATH;
    path = skip_slash(path);
    f = &super->s_root;
    dir = 0;
    name[0] = 0;

    if (pdir)
        *pdir = 0;
    *pf = 0;
    while (*path != '\0') {
        dir = f;

        // checking execute bit for cur dir
        // can't push pointer because of pointer align of packed struct
        int res;
        if ((res = access(dir->f_type, dir->f_cred, EXEC, ucred)) != 0) {
            return res;
        }

        p = path;
        while (*path != '/' && *path != '\0')
            path++;
        if (path - p >= MAXNAMELEN)
            return -E_BAD_PATH;
        memmove(name, p, path - p);
        name[path - p] = '\0';
        path = skip_slash(path);

        if (dir->f_type != FTYPE_DIR)
            return -E_NOT_FOUND;

        if ((r = dir_lookup(dir, name, &f)) < 0) {
            if (r == -E_NOT_FOUND && *path == '\0') {
                if (pdir)
                    *pdir = dir;
                if (lastelem)
                    strcpy(lastelem, name);
                *pf = 0;
            }
            return r;
        }
    }

    if (pdir)
        *pdir = dir;
    *pf = f;
    return 0;
}

/****************************************************************
 *                        File operations
 ****************************************************************/

/* Create "path".  On success set *pf to point at the file and return 0.
 * On error return < 0. */
int
file_create(const char *path, struct File **pf, int type, const struct Ucred *ucred) {
    char name[MAXNAMELEN];
    int res;
    struct File *dir, *filp;

    if (!(res = walk_path(path, &dir, &filp, name, ucred))) return res;
    if (res != -E_NOT_FOUND || dir == 0) return res;
    if ((res = dir_alloc_file(dir, &filp)) < 0) return res;


    if ((res = access(dir->f_type, dir->f_cred, WRITE, ucred)) != 0) {
        return res;
    }

    strcpy(filp->f_name, name);
    filp->parent = dir;
    filp->f_type = type;
    filp->f_cred.fc_uid = ucred->cr_uid;
    filp->f_cred.fc_gid = ucred->cr_gid;


    if (filp->f_type == FTYPE_DIR) {
        filp->f_cred.fc_permission = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;
    } else {
        filp->f_cred.fc_permission = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;
    }

    *pf = filp;
    file_flush(dir);
    return 0;
}

/* Open "path".  On success set *pf to point at the file and return 0.
 * On error return < 0. */
int
file_open(const char *path, struct File **pf, const struct Ucred *ucred, int mode) {
    int res;
    if ((res = walk_path(path, 0, pf, 0, ucred)) < 0) {
        return res;
    }

    int acc_mode = 0;

    if (mode == O_RDONLY) {
        acc_mode = READ;
    } else if (mode == O_WRONLY) {
        acc_mode = WRITE;
    } else if (mode == O_RDWR) {
        acc_mode = READ | WRITE;
    }

    if (acc_mode == 0) {
        panic("Open without any read, write flag\n");
    }

    if ((res = access((*pf)->f_type, (*pf)->f_cred, acc_mode, ucred)) < 0) {
        return res;
    }

    return 0;
}

/* Read count bytes from f into buf, starting from seek position
 * offset.  This meant to mimic the standard pread function.
 * Returns the number of bytes read, < 0 on error. */
ssize_t
file_read(struct File *f, void *buf, size_t count, off_t offset) {
    char *blk;

    if (offset >= f->f_size)
        return 0;

    count = MIN(count, f->f_size - offset);

    for (off_t pos = offset; pos < offset + count;) {
        int r = file_get_block(f, pos / BLKSIZE, &blk);
        if (r < 0) return r;

        int bn = MIN(BLKSIZE - pos % BLKSIZE, offset + count - pos);
        memmove(buf, blk + pos % BLKSIZE, bn);
        pos += bn;
        buf += bn;
    }

    return count;
}

/* Write count bytes from buf into f, starting at seek position
 * offset.  This is meant to mimic the standard pwrite function.
 * Extends the file if necessary.
 * Returns the number of bytes written, < 0 on error. */
ssize_t
file_write(struct File *f, const void *buf, size_t count, off_t offset) {
    int res;

    /* Extend file if necessary */
    if (offset + count > f->f_size)
        if ((res = file_set_size(f, offset + count)) < 0) return res;

    for (off_t pos = offset; pos < offset + count;) {
        char *blk;
        if ((res = file_get_block(f, pos / BLKSIZE, &blk)) < 0) return res;

        uint32_t bn = MIN(BLKSIZE - pos % BLKSIZE, offset + count - pos);
        memmove(blk + pos % BLKSIZE, buf, bn);
        pos += bn;
        buf += bn;
    }

    return count;
}

/* Remove a block from file f.  If it's not there, just silently succeed.
 * Returns 0 on success, < 0 on error. */
static int
file_free_block(struct File *f, uint32_t filebno) {
    blockno_t *ptr;
    int res = file_block_walk(f, filebno, &ptr, 0);
    if (res < 0) return res;

    if (*ptr) {
        free_block(*ptr);
        *ptr = 0;
    }
    return 0;
}

/* Remove any blocks currently used by file 'f',
 * but not necessary for a file of size 'newsize'.
 * For both the old and new sizes, figure out the number of blocks required,
 * and then clear the blocks from new_nblocks to old_nblocks.
 * If the new_nblocks is no more than NDIRECT, and the indirect block has
 * been allocated (f->f_indirect != 0), then free the indirect block too.
 * (Remember to clear the f->f_indirect pointer so you'll know
 * whether it's valid!)
 * Do not change f->f_size. */
static void
file_truncate_blocks(struct File *f, off_t newsize) {
    blockno_t old_nblocks = CEILDIV(f->f_size, BLKSIZE);
    blockno_t new_nblocks = CEILDIV(newsize, BLKSIZE);
    for (blockno_t bno = new_nblocks; bno < old_nblocks; bno++) {
        int res = file_free_block(f, bno);
        if (res < 0) cprintf("warning: file_free_block: %i", res);
    }

    if (new_nblocks <= NDIRECT && f->f_indirect) {
        free_block(f->f_indirect);
        f->f_indirect = 0;
    }
}

/* Set the size of file f, truncating or extending as necessary. */
int
file_set_size(struct File *f, off_t newsize) {
    if (f->f_size > newsize)
        file_truncate_blocks(f, newsize);
    f->f_size = newsize;
    flush_block(f);
    return 0;
}

/*
 * remove "path"
 * return 0 on success
 * return < 0 on error
 */
int
file_remove(const char *path, const struct Ucred *ucred) {
    struct File *rm_file, *dir;
    char last;

    int ret;
    if ((ret = walk_path(path, &dir, &rm_file, &last, ucred)) < 0) {
        return ret;
    }

    assert(strcmp(rm_file->f_name, "/") != 0);

    if ((ret = access(dir->f_type, dir->f_cred, WRITE, ucred)) < 0) {
        return ret;
    }

    // to delete a file, no permission checking of the file itself is needed.

    if (rm_file->f_type == FTYPE_DIR) {
        if (rm_file->f_size != 0) {
            return -E_NOT_EMPTY;
        }
    } else if (rm_file->f_type == FTYPE_REG) {
        file_set_size(rm_file, 0);
    } else {
        panic("Unexpected filetype in rm_file remove\n");
    }

    assert(dir->f_size != 0); // если правильно понимаю код то это так

    // move last block to free space (to keep actual f_size without segmentation)
    blockno_t nblock = dir->f_size / BLKSIZE;
    for (blockno_t i = nblock - 1; i >= 0; i--) {
        blockno_t *blockno;
        if ((file_block_walk(dir, i, &blockno, 0)) < 0) {
            panic("file_block_walk in remove");
        }
        cprintf("FS: block:%d\n", *blockno);
        struct File *f = (struct File *)diskaddr(*blockno);
        for (blockno_t j = BLKFILES - 1; j >= 0; j--) {
            if (f[j].f_name[0] != '\0' || (i == 0 && j == 0)) {
                memmove(rm_file, &f[j], sizeof(struct File));
                f[j].f_name[0] = '\0';

                if (j == 0) {
                    *blockno = 0;
                    dir->f_size -= BLKSIZE;
                }

                return 0;
            }
        }
    }
}

/* Flush the contents and metadata of file f out to disk.
 * Loop over all the blocks in file.
 * Translate the file block number into a disk block number
 * and then check whether that disk block is dirty.  If so, write it out. */
void
file_flush(struct File *f) {
    blockno_t *pdiskbno;

    for (blockno_t i = 0; i < CEILDIV(f->f_size, BLKSIZE); i++) {
        if (file_block_walk(f, i, &pdiskbno, 0) < 0 ||
            pdiskbno == NULL || *pdiskbno == 0)
            continue;
        flush_block(diskaddr(*pdiskbno));
    }
    if (f->f_indirect)
        flush_block(diskaddr(f->f_indirect));
    flush_block(f);
}

/* Sync the entire file system.  A big hammer. */
void
fs_sync(void) {
    for (int i = 1; i < super->s_nblocks; i++) {
        flush_block(diskaddr(i));
    }
}

/*
 * return 0 on success
 * otherwise < 0
 */
int
file_chmod(const char *path, permission_t perm, const struct Ucred *ucred) {
    struct File *dir, *file;
    char last;

    int res;
    if ((res = walk_path(path, &dir, &file, &last, ucred)) < 0) {
        return res;
    }

    if ((res = access(file->f_type, file->f_cred, WRITE, ucred) < 0)) {
        return res;
    }

    file->f_cred.fc_permission = perm;

    return 0;
}


/*
 * return 0 on success
 * otherwise < 0
 */
int
file_chown(const char *path, uid_t uid, const struct Ucred *ucred) {
    struct File *dir, *file;
    char last;

    int res;
    if ((res = walk_path(path, &dir, &file, &last, ucred)) < 0) {
        return res;
    }

    if ((res = access(file->f_type, file->f_cred, WRITE, ucred) < 0)) {
        return res;
    }

    file->f_cred.fc_permission = uid;
    return 0;
}

/*
 * return 0 on success
 * otherwise < 0
 */
int
file_chgrp(const char *path, gid_t gid, const struct Ucred *ucred) {
    struct File *dir, *file;
    char last;

    int res;
    if ((res = walk_path(path, &dir, &file, &last, ucred)) < 0) {
        return res;
    }

    if ((res = access(file->f_type, file->f_cred, WRITE, ucred) < 0)) {
        return res;
    }

    file->f_cred.fc_permission = gid;

    return 0;
}
